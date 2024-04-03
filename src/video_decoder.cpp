#include "opts.h"
#include "video_decoder.h"
#include <spdlog/spdlog.h>

VideoDecoder::VideoDecoder(std::shared_ptr<Context> ctx)
    : Decoder(ctx, AVMEDIA_TYPE_VIDEO) {
}

int VideoDecoder::_open(int stream_index, AVCodecContext *codec_ctx) {
    m_ctx->video_index = stream_index;
    m_ctx->video_codec_ctx = codec_ctx;
    m_ctx->video_stream = m_ctx->fmt_ctx->streams[stream_index];
    m_ctx->video_frame_rate = av_guess_frame_rate(m_ctx->fmt_ctx, m_ctx->video_stream, nullptr);
    m_queue = &m_ctx->video_packet_queue;
    return 0;
}

void VideoDecoder::run() {
    decode_loop();
}

void VideoDecoder::decode_loop() {
    int got_frame = 0;
    AVFrame *frame = av_frame_alloc();
    for (;;) {
        got_frame = decode(m_ctx->video_codec_ctx, frame);
        if (got_frame < 0) {
            break;
        }

        if (drop_frame(frame)) {
            continue;
        }
        // TODO add video filters, filter may add while 
        // TODO 设置 frame_last_filter_delay
        
        if (!enqueue_frame(frame)) {
            break;
        }
        // av_frame_unref(frame);
        if (m_ctx->video_packet_queue.serial() != m_pkt_serial) {
            spdlog::warn("the serial in video packet queue and decoder is different, serial={}, serial={}", 
                m_ctx->video_packet_queue.serial(), m_pkt_serial);
            break;
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
    // "-framedrop"选项用于设置当视频帧失去同步时，是否丢弃视频帧。"-framedrop"选项以bool方式改变变量framedrop值。
    // 音视频同步方式有三种：A同步到视频，B同步到音频，C同步到外部时钟。
    // 1) 当命令行不带"-framedrop"选项或"-noframedrop"时，framedrop值为默认值-1，若同步方式是"同步到视频"
    //    则不丢弃失去同步的视频帧，否则将丢弃失去同步的视频帧。
    // 2) 当命令行带"-framedrop"选项时，framedrop值为1，无论何种同步方式，均丢弃失去同步的视频帧。
    // 3) 当命令行带"-noframedrop"选项时，framedrop值为0，无论何种同步方式，均不丢弃失去同步的视频帧。
    if ((framedrop > 0 || (framedrop && m_ctx->master_clock->sync_type() != SYNC_TYPE_VIDEO)) && frame->pts != AV_NOPTS_VALUE) {
        double diff = dpts - m_ctx->master_clock->get();
        if (!isnan(diff) &&
            fabs(diff) < AV_NOSYNC_THRESHOLD &&
            diff - m_ctx->frame_last_filter_delay < 0 &&
            m_pkt_serial == m_ctx->video_clock.serial() &&
            m_ctx->video_packet_queue.count())
        {
            m_ctx->frame_drops_early++;
            av_frame_unref(frame); // 视频帧失去同步则直接扔掉
            return true;
        }
    }
    return false;
}

bool VideoDecoder::enqueue_frame(AVFrame* frame) {
    Frame* vp = m_ctx->video_frame_queue.peek_writable();
    if (!vp) {
        return false;
    }


    AVRational tb = m_ctx->video_stream->time_base;
    AVRational frame_rate = av_guess_frame_rate(m_ctx->fmt_ctx, m_ctx->video_stream, NULL);

    vp->uploaded = 0;
    vp->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);// frame->pts;
    vp->duration = (frame_rate.num && frame_rate.den ? av_q2d(AVRational{ frame_rate.den, frame_rate.num }) : 0);//frame->duration;
    vp->pos = frame->pkt_pos;
    vp->serial = m_pkt_serial;

    av_frame_move_ref(vp->frame, frame);
    m_ctx->video_frame_queue.push();
    return true;
}