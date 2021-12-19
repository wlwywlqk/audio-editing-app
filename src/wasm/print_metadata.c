#include <stdio.h>
#include <emscripten.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

int read_packet(void *opaque, uint8_t *buffer, int buffer_lengthbuf_size);

int main() {
    printf("main\n");
    return 0;
}

static struct {
    uint8_t *p;
    size_t len;
} buffer_data;


EMSCRIPTEN_KEEPALIVE
void print_metadata (uint8_t *buffer, const int buffer_length) {
    printf("print_metadata %d\n", buffer_length);

    buffer_data.p = buffer;
    buffer_data.len = buffer_length;

    AVFormatContext *format_ctx = avformat_alloc_context();
    const AVDictionaryEntry *tag = NULL;

    uint8_t *buffer_p = (uint8_t *)av_malloc(buffer_length);


    AVIOContext *avio_ctx = avio_alloc_context(buffer_p, buffer_length, 0, NULL, read_packet, NULL, NULL);

    format_ctx->pb = avio_ctx;
    format_ctx->flags = AVFMT_FLAG_CUSTOM_IO;

    int err;
    if ((err = avformat_open_input(&format_ctx, "", NULL, NULL)) < 0) {
        fprintf(stderr, "open input err %d\n", err);
        return;
    }
     if ((err = avformat_find_stream_info(format_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "find stream info %d\n", err);
        return;
    }

    while ((tag = av_dict_get(format_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        printf("%s=%s\n", tag->key, tag->value);

    av_dump_format(format_ctx, 0, "", 0);

    avformat_close_input(&format_ctx);
    av_free(avio_ctx->buffer);
    av_free(avio_ctx);
    av_free(buffer_p);
}

int read_packet(void *opaque, uint8_t *buffer, int buffer_length) {

    buffer_length = FFMIN(buffer_length, buffer_data.len);

    if (!buffer_length)
        return AVERROR_EOF;
    memcpy(buffer, buffer_data.p, buffer_length);
    buffer_data.p += buffer_length;
    buffer_data.len -= buffer_length;

    return buffer_length;
}