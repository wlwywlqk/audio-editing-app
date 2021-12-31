#include "index.h"

uint8_t *concat(uint8_t *buffer_list[], size_t size_list[], const int count) {
    int i, ret = 0;
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *io_ctx = NULL;

    uint8_t *buffer = NULL;


    for (i = 0; i < count; i++) {

        buffer = av_malloc(size_list[i]);
        memcpy(buffer, buffer_list[i], size_list[i]);

        if (!(fmt_ctx = avformat_alloc_context())) {
            ret = AVERROR(ENOMEM);
            goto end;
        }


        if (!(io_ctx = avio_alloc_context(buffer, size_list[i], 0, NULL, NULL, NULL, NULL))) {
            ret = AVERROR(ENOMEM);
            goto end;
        }
        fmt_ctx->pb = io_ctx;
        ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
        if (ret < 0) {
            printf("Could not open input\n");
            goto end;
        }
        
        ret = avformat_find_stream_info(fmt_ctx, NULL);
        if (ret < 0) {
            printf("Could not find stream information\n");
            goto end;
        }

        av_dump_format(fmt_ctx, 0, NULL, 0);
    }


end:
    avformat_close_input(&fmt_ctx);

    if (ret < 0) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return NULL;
    }

    return NULL;
}