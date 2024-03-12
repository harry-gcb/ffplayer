#include "opts.h"
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

void VideoDecoder::run() {
    decode_loop();
}

void VideoDecoder::decode_loop() {
    int got_frame = 0;
    AVFrame *frame = av_frame_alloc();
    for (;;) {
        got_frame = decode(m_ctx->audio_codec_ctx, frame);
        if (got_frame < 0) {
            break;
        }
        if (!got_frame) {
            continue;
        }
        if (drop_frame(frame)) {
            continue;
        }



    }

    av_frame_free(&frame);
    return ;
}

bool VideoDecoder::drop_frame(AVFrame *frame) {
    double dpts = NAN;
    if (frame->pts != AV_NOPTS_VALUE) {
        dpts = av_q2d(m_ctx->video_stream->time_base) * frame->pts;
    }
    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(m_ctx->fmt_ctx, m_ctx->video_stream, frame);
    if (framedrop > 0 && m_ctx->sync_type != SYNC_TYPE_VIDEO && frame->pts != AV_NOPTS_VALUE) {
        double diff = dpts - m_ctx->master_clock->get();
        if (!isnan(diff) &&
            fabs(diff) < AV_NOSYNC_THRESHOLD &&
            diff - m_ctx->frame_last_filter_delay < 0 &&
            m_pkt_serial == m_ctx->video_clock.serial() &&
            m_ctx->video_packet_queue.count())
            m_ctx->frame_drops_early++;
            av_frame_unref(frame);
            return true;
    }
    return false;
}