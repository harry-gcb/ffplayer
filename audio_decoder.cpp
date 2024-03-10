#include "audio_decoder.h"

AudioDecoder::AudioDecoder(std::shared_ptr<Context> ctx)
    : Decoder(ctx, AVMEDIA_TYPE_AUDIO) {
}

int AudioDecoder::_open(int stream_index, AVCodecContext *codec_ctx) {
    m_ctx->audio_index = stream_index;
    m_ctx->audio_codec_ctx = codec_ctx;
    m_ctx->audio_stream = m_ctx->fmt_ctx->streams[stream_index];
    return 0;
}