#include "index.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include <sys/stat.h>



struct buffer_data {
    uint8_t *ptr;
    size_t size;
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, bd->size);
    if (!buf_size)
        return AVERROR_EOF;

    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;

    return buf_size;
}

static int write_packet(void *opaque, uint8_t *buf, int buf_size) {
    struct buffer_data *bd = (struct buffer_data *)opaque;
    printf("%d\n", buf_size);

    if (!buf_size)
        return AVERROR_EOF;

    bd->ptr = av_realloc(bd->ptr, bd->size + buf_size);

    if (!bd->ptr) {
        return AVERROR(ENOMEM);
    }
    memcpy(buf, bd->ptr += bd->size, buf_size);


    return buf_size;
}

uint8_t *concat(uint8_t *buffer_list[], size_t size_list[], const int count) {
    int i, ret = 0;
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *io_ctx = NULL, *o_io_ctx = NULL;
    AVPacket pkt;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t avio_ctx_buffer_size = 4096, o_size = 0;
    struct buffer_data bd = { 0 }, o_bd = {};

    AVFormatContext **fmt_ctx_array = NULL;

    fmt_ctx_array = av_malloc_array(count, sizeof(*fmt_ctx));
    if (!fmt_ctx_array) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        goto end;
    }


    AVFormatContext *o_fmt_ctx = avformat_alloc_context();

    if (!o_fmt_ctx) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    o_bd.ptr = av_malloc(0);
    if (!o_bd.ptr) {
        ret = AVERROR(ENOMEM);
        goto end;
    }


    if (!(o_io_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 1, &o_bd, NULL, &write_packet, NULL))) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    ret = avformat_alloc_output_context2(&o_fmt_ctx, NULL, "mp3", NULL);
    o_fmt_ctx->pb = o_io_ctx;

    if (ret < 0) {
        printf("Could not open out input\n");
        goto end;
    }

   
    for (i = 0; i < count; i++) {

        bd.ptr  = buffer_list[i];
        bd.size = size_list[i];


        if (!(fmt_ctx = avformat_alloc_context())) {
            ret = AVERROR(ENOMEM);
            goto end;
        }
        fmt_ctx_array[i] = fmt_ctx;

        

        if (!(io_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, &bd, &read_packet, NULL, NULL))) {
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



        AVStream *o_stream = avformat_new_stream(o_fmt_ctx, NULL);
        if (!o_stream) {
            printf("Could not new output stream\n");
            goto end;
        }
        ret = avcodec_parameters_copy(o_stream->codecpar, fmt_ctx->streams[i]->codecpar);

        if (ret < 0) {
            printf("Could not copy codec parameters\n");
            goto end;
        }
        o_stream->codecpar->codec_tag = 0;



        avformat_close_input(&fmt_ctx);
        avio_context_free(&io_ctx);
    }

    ret = avformat_write_header(o_fmt_ctx, NULL);
    if (ret < 0) {
        printf("Could not write header for output file\n");
        goto end;
    }

    ret = av_write_trailer(o_fmt_ctx);

    if (ret < 0) {
        printf("Could not write trailer for output file\n");
        goto end;
    }

end:
    avformat_free_context(o_fmt_ctx);

    if (ret < 0) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return NULL;
    }

    return o_bd.ptr;
}