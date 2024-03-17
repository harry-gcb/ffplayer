#include "audio_decoder.h"
#include <spdlog/spdlog.h>

AudioDecoder::AudioDecoder(std::shared_ptr<Context> ctx)
    : Decoder(ctx, AVMEDIA_TYPE_AUDIO) {
}

int AudioDecoder::_open(int stream_index, AVCodecContext *codec_ctx) {
    m_ctx->audio_index = stream_index;
    m_ctx->audio_codec_ctx = codec_ctx;
    m_ctx->audio_stream = m_ctx->fmt_ctx->streams[stream_index];
    m_ctx->audio_codec_ctx->pkt_timebase = m_ctx->audio_stream->time_base;
    m_start_pts = m_ctx->audio_stream->start_time;
    m_start_pts_tb = m_ctx->audio_stream->time_base;
    m_queue = &m_ctx->audio_packet_queue;
    return 0;
}

int AudioDecoder::_close() {
    if (m_ctx->audio_codec_ctx) {
        avcodec_free_context(&m_ctx->audio_codec_ctx);
        m_ctx->audio_codec_ctx = nullptr;
    }
    return 0;
}

void AudioDecoder::run() {
    decode_loop();
}

void AudioDecoder::decode_loop() {
    int got_frame = 0;
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        spdlog::error("av_frame_alloc failed");
        return;
    }
    for (;;) {
        got_frame = decode(m_ctx->audio_codec_ctx, frame);
        if (got_frame < 0) {
            break;
        }
        if (!enqueue_frame(frame)) {
            break;
        }

        if (m_ctx->audio_packet_queue.serial() != m_pkt_serial) {
            break;
        }
    };

    av_frame_free(&frame);
}

bool AudioDecoder::enqueue_frame(AVFrame* frame) {
    Frame* af = m_ctx->audio_frame_queue.peek_writable();
    if (!af) {
        return false;
    }
    af->pts = frame->pts;
    af->pos = frame->pkt_pos;
    af->serial = m_pkt_serial;
    af->duration = frame->duration;

    av_frame_move_ref(af->frame, frame);
    m_ctx->audio_frame_queue.push();
    return true;
}