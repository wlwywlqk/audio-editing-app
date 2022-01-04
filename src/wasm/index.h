#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavformat/avio.h>


typedef struct buffer_data {
  uint8_t *ptr;
  size_t size;
} buffer_data;
typedef struct o_buffer_data {
    size_t size;
    uint8_t *ptr;
    size_t capacity;
} o_buffer_data;

o_buffer_data *concat(uint8_t *buffer[], size_t size[], const int count);