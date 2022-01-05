#include "index.h"

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
  struct buffer_data *bd = (struct buffer_data *)opaque;
  buf_size = FFMIN(buf_size, bd->size);

  if (!buf_size)
    return AVERROR_EOF;

  memcpy(buf, bd->ptr, buf_size);
  bd->ptr += buf_size;
  bd->size -= buf_size;

  return buf_size;
}

static int write_packet(void *opaque, uint8_t *buf, int buf_size) {
  o_buffer_data *bd = (struct o_buffer_data *)opaque;
  size_t size = bd->size + buf_size;

  if (!buf_size)
    return AVERROR_EOF;

  if (bd->capacity < size) {
    bd->ptr = av_realloc(bd->ptr, size * 2);

    if (!bd->ptr) {
      return AVERROR(ENOMEM);
    }
    bd->capacity = size * 2;
  }

  memcpy(bd->ptr + bd->size, buf, buf_size);
  bd->size = size;
  return buf_size;
}

int dump_buffer(uint8_t *buffer, size_t size) {
  int ret = 0;
  AVFormatContext *fmt_ctx = NULL;
  AVIOContext *io_ctx = NULL;
  AVPacket pkt;
  uint8_t *avio_ctx_buffer = NULL;
  size_t avio_ctx_buffer_size = 4096;
  buffer_data bd = {0};
  bd.ptr = buffer;
  bd.size = size;

  avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
  if (!avio_ctx_buffer) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(fmt_ctx = avformat_alloc_context())) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(io_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0,
                                    &bd, &read_packet, NULL, NULL))) {
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

end:
  avformat_close_input(&fmt_ctx);
  if (ret < 0) {
    fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
    return ret;
  }

  return 0;
}

o_buffer_data *concat(uint8_t *buffer_list[], size_t size_list[],
                      const int count) {
  int i, j, ret = 0;
  int64_t pts_offset = 0, dts_offset = 0;
  AVFormatContext *fmt_ctx = NULL;
  AVIOContext *io_ctx = NULL, *o_io_ctx = NULL;
  AVPacket pkt;
  uint8_t *o_avio_ctx_buffer = NULL;
  size_t avio_ctx_buffer_size = 4096;
  o_buffer_data *o_bd = NULL;

  o_bd = av_malloc(sizeof(o_buffer_data));
  if (!o_bd) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  AVFormatContext **fmt_ctx_array = NULL;

  fmt_ctx_array = av_malloc_array(count, sizeof(*fmt_ctx));
  if (!fmt_ctx_array) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  buffer_data *bd_array = NULL;

  bd_array = av_malloc_array(count, sizeof(buffer_data));
  if (!bd_array) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  o_avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
  if (!o_avio_ctx_buffer) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  AVFormatContext *o_fmt_ctx = avformat_alloc_context();

  if (!o_fmt_ctx) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  o_bd->ptr = av_malloc(avio_ctx_buffer_size);
  o_bd->capacity = avio_ctx_buffer_size;
  o_bd->size = 0;
  if (!o_bd->ptr) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(o_io_ctx = avio_alloc_context(o_avio_ctx_buffer, avio_ctx_buffer_size,
                                      1, o_bd, NULL, &write_packet, NULL))) {
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
    uint8_t *avio_ctx_buffer = NULL;

    bd_array[i].ptr = buffer_list[i];
    bd_array[i].size = size_list[i];

    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
      ret = AVERROR(ENOMEM);
      goto end;
    }

    if (!(fmt_ctx = avformat_alloc_context())) {
      ret = AVERROR(ENOMEM);
      goto end;
    }
    fmt_ctx_array[i] = fmt_ctx;

    if (!(io_ctx =
              avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0,
                                 &bd_array[i], &read_packet, NULL, NULL))) {
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

    if (i == 0) {
      for (j = 0; j < fmt_ctx->nb_streams; j++) {
        AVStream *o_stream = avformat_new_stream(o_fmt_ctx, NULL);
        if (!o_stream) {
          printf("Could not new output stream\n");
          goto end;
        }
        ret = avcodec_parameters_copy(o_stream->codecpar,
                                      fmt_ctx->streams[j]->codecpar);

        if (ret < 0) {
          printf("Could not copy codec parameters\n");
          goto end;
        }
        o_stream->codecpar->codec_tag = 0;
      }
    }
  }

  ret = avformat_write_header(o_fmt_ctx, NULL);
  if (ret < 0) {
    printf("Could not write header for output file\n");
    goto end;
  }

  for (i = 0; i < count; i++) {
    fmt_ctx = fmt_ctx_array[i];
    int64_t pts = 0, dts = 0, duration = 0;


    while (1) {
      AVStream *i_stream, *o_stream;

      ret = av_read_frame(fmt_ctx, &pkt);
      if (ret < 0) {
        break;
      }
      i_stream = fmt_ctx->streams[pkt.stream_index];
      o_stream = o_fmt_ctx->streams[pkt.stream_index];
      if (!o_stream) {
        av_packet_unref(&pkt);
        continue;
      }

      pkt.pts =
          pts_offset + av_rescale_q_rnd(pkt.pts, i_stream->time_base, o_stream->time_base,
                           AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
      pkt.dts =
          dts_offset + av_rescale_q_rnd(pkt.dts, i_stream->time_base, o_stream->time_base,
                           AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
      pkt.duration =
          av_rescale_q(pkt.duration, i_stream->time_base, o_stream->time_base);
      pkt.pos = -1;

      pts = pkt.pts;
      dts = pkt.dts;
      duration = pkt.duration;

      ret = av_interleaved_write_frame(o_fmt_ctx, &pkt);
      if (ret < 0) {
        break;
      }

      av_packet_unref(&pkt);
    }

    pts_offset = pts + duration;
    dts_offset = dts + duration;

    printf("%d   %d\n", pts_offset, dts_offset);

    av_packet_unref(&pkt);

    avformat_close_input(&fmt_ctx);
  }

  ret = av_write_trailer(o_fmt_ctx);

  if (ret < 0) {
    printf("Could not write trailer for output file\n");
    goto end;
  }

  av_dump_format(o_fmt_ctx, 0, NULL, 1);

end:
  if (o_avio_ctx_buffer) {
    av_freep(&o_avio_ctx_buffer);
  }

  if (o_io_ctx) {
    avio_context_free(&o_io_ctx);
  }

  if (o_fmt_ctx) {
    avformat_free_context(o_fmt_ctx);
  }

  if (fmt_ctx_array) {
    av_freep(&fmt_ctx_array);
  }
  if (bd_array) {
    av_freep(&bd_array);
  }

  if (ret < 0) {
    fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
    return NULL;
  }

  return o_bd;
}