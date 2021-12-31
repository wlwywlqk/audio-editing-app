#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavformat/avio.h>



uint8_t *concat(uint8_t *buffer[], size_t size[], const int count);