/*
 * Reads a file in binary mode, splits it into 4 KiB segments,
 * and prints each segment's index, byte count, and first 8 bytes.
 * 
 * Compilation (Linux/Unix):
 *   gcc -std=c99 -D_POSIX_C_SOURCE=200112L -o bin_seg bin_seg.c
 * 
 * Compilation (Windows with MinGW):
 *   gcc -std=c99 -o bin_seg.exe bin_seg.c
 */

#include "bin_seg.h"  /* our header with Segment and SegmentReader definitions */
#include <stdio.h> // for fopen, fread, printf
#include <stdlib.h> // for malloc, free
#include <stdint.h> // this is gonna give us fixed‑size integer types like uint32_t (unsigned 32‑bit), same on all platforms.
#include <string.h> // safe side
#include <errno.h> // to get the error codes from the os

/* Make fseeko / ftello available on POSIX systems */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200112L
#include <sys/stat.h>
#endif

/*
 Why not store all segments in an array?
 In the original version, if the file is 1 GB, 
 you’d have ~250,000 segments in memory → 1 GB of RAM.
 That’s wasteful and impossible for huge files. 
 this version only stores one segment at a time.
*/

/* 
 * SegmentReader – opaque handle that keeps track of the file and next segment index.
 * It allows us to read segments one by one (streaming) without loading all at once.
 */
struct SegmentReader {
    FILE *fp;
    uint32_t next_index;
    int error_flag;
};

/* Error codes for SegmentReader */
#define SEG_READER_OK          0
#define SEG_READER_OPEN_ERR    1
#define SEG_READER_SEEK_ERR    2
#define SEG_READER_READ_ERR    3

int segment_reader_get_error(const SegmentReader *rd) {
    return rd ? rd->error_flag : SEG_READER_OPEN_ERR;
}

/* 1. Open the file and prepare a SegmentReader*/

SegmentReader* segment_reader_open(const char *path) {
    SegmentReader *rd = (SegmentReader*) malloc(sizeof(SegmentReader));
    if (!rd) {
        return NULL;              /* malloc failed */
    }

    rd->fp = fopen(path, "rb");
    if (!rd->fp) {
        free(rd);
        return NULL;
    }

    rd->next_index = 0;
    rd->error_flag = SEG_READER_OK;

    /* (Optional) Verify we can seek – not required for sequential reading,
       but useful to detect if the file is a pipe or socket. */
    if (fseek(rd->fp, 0, SEEK_CUR) != 0) {
        rd->error_flag = SEG_READER_SEEK_ERR;
    }

    return rd;
}

 
/* 2. Read the next segment from the file*/

int segment_reader_read_next(SegmentReader *rd, Segment *out) {
    if (!rd || !out) return 0;
    if (rd->error_flag != SEG_READER_OK) return 0;

    size_t total_read = 0;
    /* Keep reading until we have SEGMENT_SIZE bytes or hit EOF/error */
    while (total_read < SEGMENT_SIZE) {
        size_t bytes = fread(out->data + total_read, 1,
                             SEGMENT_SIZE - total_read, rd->fp);
        if (bytes == 0) {
            /* End of file or error */
            if (feof(rd->fp)) break;     /* normal EOF, partial last segment */
            if (ferror(rd->fp)) {
                rd->error_flag = SEG_READER_READ_ERR;
                return 0;
            }
        }
        total_read += bytes;
    }

    out->index = rd->next_index++;
    out->bytes_read = total_read;

    return (total_read > 0) ? 1 : 0;   /* 1 = segment was read, 0 = no more data */
}


/* 3. Close the reader and free memory */

void segment_reader_close(SegmentReader *rd) {
    if (rd) {
        if (rd->fp) fclose(rd->fp);
        free(rd);
    }
}


/* 4. Print a single segment's metadata and first few bytes  */

void segment_print(const Segment *seg) {
    printf("Segment %4u | %zu bytes | first %d bytes: ",
           seg->index, seg->bytes_read, BYTES_TO_SHOW);

    size_t limit = (BYTES_TO_SHOW < seg->bytes_read) ? BYTES_TO_SHOW : seg->bytes_read;
    for (size_t j = 0; j < limit; j++) {
        printf("%02x ", seg->data[j]);
    }
    printf("\n");
}


/* 5. Convert error code to human-readable string                            */

const char* segment_reader_error_str(int err) {
    switch (err) {
        case SEG_READER_OK:        return "No error";
        case SEG_READER_OPEN_ERR:  return "Failed to open file";
        case SEG_READER_SEEK_ERR:  return "File is not seekable (pipe or socket)";
        case SEG_READER_READ_ERR:  return "Read error";
        default:                   return "Unknown error";
    }
}

