#include "decoder.h"
#include <spdlog/spdlog.h>

Decoder::Decoder(std::shared_ptr<Context> ctx, AVMediaType media_type)
    : m_ctx(ctx),
      m_media_type(media_type)
{
}

int Decoder::open() {
    const AVCodec *codec = nullptr;
    AVFormatContext *fmt_ctx = m_ctx->fmt_ctx;
    int stream_index = av_find_best_stream(fmt_ctx, m_media_type, -1, -1, &codec, 0);
    if (stream_index < 0 || stream_index >= (int)fmt_ctx->nb_streams) {
        spdlog::error("av_find_best_stream failed, stream_index={}", stream_index);
        return -1;
    }
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[stream_index]->codecpar);
    int ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        spdlog::error("avcodec_open2 failed, ret={}", ret);
        return -1;
    }
    return _open(stream_index, codec_ctx);
}

int Decoder::_open(int stream_index, AVCodecContext *codec_ctx) {
    return 0;
}