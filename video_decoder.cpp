#include "video_decoder.h"

VideoDecoder::VideoDecoder(std::shared_ptr<Context> ctx)
    : Decoder(ctx, AVMEDIA_TYPE_VIDEO) {
}

int VideoDecoder::_open(int stream_index, AVCodecContext *codec_ctx) {
    m_ctx->video_index = stream_index;
    m_ctx->video_codec_ctx = codec_ctx;
    m_ctx->video_stream = m_ctx->fmt_ctx->streams[stream_index];
    return 0;
}