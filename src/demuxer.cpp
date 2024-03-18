#include "demuxer.h"
#include "opts.h"
#include <spdlog/spdlog.h>

Demuxer::Demuxer(std::shared_ptr<Context> ctx)
    : m_ctx(ctx)
{
}

Demuxer::~Demuxer() {
    if (m_ctx->fmt_ctx) {
        avformat_free_context(m_ctx->fmt_ctx);
        m_ctx->fmt_ctx = nullptr;
    }
}

int Demuxer::open() {
    int ret = avformat_open_input(&m_ctx->fmt_ctx, m_ctx->filename, m_ctx->iformat, nullptr);
    if (ret < 0) {
        spdlog::error("avformat_open_input failed, ret={}", ret);
        return -1;
    }
    ret = avformat_find_stream_info(m_ctx->fmt_ctx, nullptr);
    if (ret < 0) {
        spdlog::error("avformat_find_stream_info failed, ret={}", ret);
        return -1;
    }
    m_ctx->max_frame_duration = (m_ctx->fmt_ctx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    av_dump_format(m_ctx->fmt_ctx, 0, m_ctx->filename, 0);
    return 0;
}

int Demuxer::close() {
    if (m_ctx->fmt_ctx) {
        avformat_free_context(m_ctx->fmt_ctx);
        m_ctx->fmt_ctx = nullptr;
    }
    return 0;
}

void Demuxer::run() {
    demux_loop();
}

void Demuxer::demux_loop() {
    int ret = 0;
    bool last_paused = false;
    AVPacket *pkt = av_packet_alloc();
    for (;;) {
        if (m_stop) {
            spdlog::info("request quit while demux loop");
            break;
        }
        if (m_ctx->paused != last_paused) {
            last_paused = m_ctx->paused;
            if (m_ctx->paused) {
                av_read_pause(m_ctx->fmt_ctx);
            } else {
                av_read_play(m_ctx->fmt_ctx);
            }
        }
        // TODO rtsp paused

        if (m_ctx->seek_req) {
            
            int64_t seek_target = m_ctx->seek_pos;
            int64_t seek_min = m_ctx->seek_rel > 0 ? seek_target - m_ctx->seek_rel + 2 : INT64_MIN;
            int64_t seek_max = m_ctx->seek_rel < 0 ? seek_target - m_ctx->seek_rel - 2 : INT64_MAX;
            ret = avformat_seek_file(m_ctx->fmt_ctx, -1, seek_min, seek_target, seek_max, m_ctx->seek_flags);
            if (ret < 0) {
                spdlog::warn("avformat_seek_file failed, ret={}", ret);
            } else {
                // 清空packet队列
                if (m_ctx->audio_index >= 0) {
                    m_ctx->audio_packet_queue.flush();
                }
                if (m_ctx->video_index >= 0) {
                    m_ctx->video_packet_queue.flush();
                }
                if (m_ctx->subtitle_index >= 0) {
                    m_ctx->subtitle_packet_queue.flush();
                }
                // TODO 重置外部时钟
                // if (m_ctx->seek_flags & AVSEEK_FLAG_BYTE) {
                //     m_ctx->extern_clock.set(NAN, 0);
                // } else {
                //     m_ctx->extern_clock.set(seek_target / (double)AV_TIME_BASE, 0);
                // }
            }
            m_ctx->seek_req = false;
            // m_ctx->eof = 0;
            // 如果暂停中执行seek操作，取消暂停，执行后续步骤
            // if (is->paused) {
            //     step_to_next_frame();
            // }
        }

        // TODO queue_attachments_req

        if (m_ctx->audio_packet_queue.size() +
            m_ctx->video_packet_queue.size() +
            m_ctx->subtitle_packet_queue.size() > MAX_QUEUE_SIZE ||
            m_ctx->audio_packet_queue.count() > MIN_FRAMES ||
            m_ctx->video_packet_queue.count() > MIN_FRAMES ||
            m_ctx->subtitle_packet_queue.count() > MIN_FRAMES) {
            std::unique_lock<std::mutex> lock{m_ctx->demux_mutex};
            m_ctx->demux_cond.wait_for(lock, std::chrono::milliseconds(10));
            continue;
        }

        // TODO !paused and play down

        ret = av_read_frame(m_ctx->fmt_ctx, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(m_ctx->fmt_ctx->pb)) && !m_ctx->eof) {
                if (m_ctx->video_index >= 0) {
                    m_ctx->video_packet_queue.put(pkt);
                }
                if (m_ctx->audio_index >= 0) {
                    m_ctx->audio_packet_queue.put(pkt);
                }
                if (m_ctx->subtitle_index >= 0) {
                    m_ctx->subtitle_packet_queue.put(pkt);
                }
                m_ctx->eof = 1;
            }
            // TODO eof
            // if (ic->pb && ic->pb->error)
            // {
            //     if (autoexit)
            //         goto fail;
            //     else
            //         break;
            // }

            // 文件已经读完了，等待解码线程后续操作
            std::unique_lock<std::mutex> lock{m_ctx->demux_mutex};
            m_ctx->demux_cond.wait_for(lock, std::chrono::milliseconds(10));
            continue;
        } else {
            m_ctx->eof = 0;
        }
        // spdlog::info("av_read_frame a packet, ret={}, stream_index={}", ret, pkt->stream_index);
        // TODO pkt_in_play_range
        if (pkt->stream_index == m_ctx->audio_index) {
            m_ctx->audio_packet_queue.put(pkt);
        } else if (pkt->stream_index == m_ctx->video_index) {
            m_ctx->video_packet_queue.put(pkt);
        } else if (pkt->stream_index == m_ctx->subtitle_index) {
            m_ctx->subtitle_packet_queue.put(pkt);
        } else {
            av_packet_unref(pkt);
        }
    }
    av_packet_free(&pkt);
}