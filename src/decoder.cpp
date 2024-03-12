#include "decoder.h"
#include <spdlog/spdlog.h>

Decoder::Decoder(std::shared_ptr<Context> ctx, AVMediaType media_type)
    : m_ctx(ctx),
      m_media_type(media_type),
      m_pkt(av_packet_alloc())
{
}

Decoder::~Decoder() {
    if (m_pkt) {
        av_packet_free(&m_pkt);
        m_pkt = nullptr;
    }
    close();
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

int Decoder::close() {
    return _close();
}

int Decoder::decode(AVCodecContext *codec_ctx, AVFrame *frame) {
    int ret = AVERROR(EAGAIN);

    for (;;) {
        if (m_queue->serial() == m_pkt_serial) {
            do {
                if (m_queue->request_aborted()) {
                    return -1;
                }
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret >= 0) {
                    if (AVMEDIA_TYPE_AUDIO == codec_ctx->codec_type) {
                        AVRational tb = AVRational{1, frame->sample_rate};
                        if (frame->pts != AV_NOPTS_VALUE) {
                            frame->pts = av_rescale_q(frame->pts, codec_ctx->pkt_timebase, tb);
                        } else if (m_next_pts != AV_NOPTS_VALUE) {
                            frame->pts = av_rescale_q(m_next_pts, m_next_pts_tb, tb);
                        }
                        if (frame->pts != AV_NOPTS_VALUE) {
                            m_next_pts = frame->pts + frame->nb_samples;
                            m_next_pts_tb = tb;
                        }
                    } else if (AVMEDIA_TYPE_VIDEO == codec_ctx->codec_type) {
                        if (m_recoder_pts == -1) {
                            frame->pts = frame->best_effort_timestamp;
                        } else if (!m_recoder_pts) {
                            frame->pts = frame->pkt_dts;
                        }
                    }
                }

                if (AVERROR_EOF == ret) {
                    m_finished = m_pkt_serial;
                    avcodec_flush_buffers(codec_ctx);
                    return 0;
                }

                if (ret >= 0) {
                    return 1;
                }
            } while (ret != AVERROR(EAGAIN));
        }

        do {
            if (m_queue->count() == 0) {
                m_ctx->demux_cond.notify_one();
            }
            if (m_packet_pending) {
                m_packet_pending = 0;
            } else {
                int old_serial = m_pkt_serial;
                if (m_queue->get(m_pkt, 1, m_pkt_serial) < 0) {
                    return -1;
                }
                if (old_serial != m_pkt_serial) {
                    avcodec_flush_buffers(codec_ctx);
                    m_finished = 0;
                    m_next_pts = m_start_pts;
                    m_next_pts_tb = m_start_pts_tb;
                }
            }
            if (m_queue->serial() == m_pkt_serial) {
                break;
            }
            av_packet_unref(m_pkt);
        } while (1);

        // TODO subtitle

        // TODO FrameData

        if (avcodec_send_packet(codec_ctx, m_pkt) == AVERROR(EAGAIN)) {
            m_packet_pending = 1;
        } else {
            av_packet_unref(m_pkt);
        }
    }
}

int Decoder::_open(int stream_index, AVCodecContext *codec_ctx) {
    return 0;
}

int Decoder::_close() {
    return 0;
}