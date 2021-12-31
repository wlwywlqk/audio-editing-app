#include <stdio.h>
#include <libavutil/file.h>
#include "index.h"


int main(int argc, char *argv[]) {
    int ret = 0;
    uint8_t * buffer_list[1] = { NULL };
    size_t size_list[1] = { 1 };
    char *filename = argv[1];

    uint8_t *buffer = NULL;
    size_t size = 0;



    ret = av_file_map(filename, &buffer, &size, 0, NULL);

    buffer_list[0] = buffer;
    size_list[0] = size;
    concat(buffer_list, size_list, 1);
    return ret;
}