#ifndef BIN_SEG_H
#define BIN_SEG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#define SEGMENT_SIZE     4096   
#define BYTES_TO_SHOW    8     

#define SEG_READER_OK          0
#define SEG_READER_OPEN_ERR    1
#define SEG_READER_SEEK_ERR    2
#define SEG_READER_READ_ERR    3

typedef struct {
    uint32_t index;               
    size_t   bytes_read;           
    uint8_t  data[SEGMENT_SIZE];   
} Segment;

typedef struct SegmentReader SegmentReader;

SegmentReader* segment_reader_open(const char *path);
int segment_reader_read_next(SegmentReader *rd, Segment *out);
void segment_reader_close(SegmentReader *rd);
int segment_reader_get_error(const SegmentReader *rd);
const char* segment_reader_error_str(int err);
void segment_print(const Segment *seg);

#endif