#ifndef FFPLAYER_CONTEXT_H_
#define FFPLAYER_CONTEXT_H_

#include <atomic>
#include <mutex>
#include <condition_variable>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include "clock.h"
#include "packet_queue.h"
#include "frame_queue.h"

#define MAX_QUEUE_SIZE (16*1024*1024)
#define MIN_FRAMES 10000

class Context {
    friend class Player;
    friend class Demuxer;
    friend class Decoder;
    friend class AudioDecoder;
    friend class VideoDecoder;
    friend class AudioPlayer;
    friend class VideoPlayer;
public:
    Context(const char *filename);
    ~Context();

private:
    const char *filename = nullptr; // 文件名

    AVFormatContext *fmt_ctx = nullptr; // 解封装器上下文

    int             audio_index = -1;          // 音频流索引
    AVCodecContext *audio_codec_ctx = nullptr; // 音频流解码器上下文
    AVStream       *audio_stream = nullptr;    // 音频流
    PacketQueue     audio_packet_queue;        // 音频packet队列
    Clock           audio_clock{&audio_packet_queue.m_serial, SYNC_TYPE_AUDIO};               // 音频时钟
    FrameQueue      audio_frame_queue{ &audio_packet_queue ,AUDIO_FRAME_QUEUE_SIZE, 1 }; //音频帧队列
    SwrContext     *audio_swr_ctx = nullptr;

    int             video_index = -1;          // 视频流索引
    AVCodecContext *video_codec_ctx = nullptr; // 视频流解码器上下文
    AVStream       *video_stream = nullptr;    // 视频流
    PacketQueue     video_packet_queue;        // 视频packet队列
    Clock           video_clock{&video_packet_queue.m_serial, SYNC_TYPE_VIDEO};               // 视频时钟
    AVRational      video_frame_rate;          // 视频帧率
    FrameQueue      video_frame_queue{ &video_packet_queue, VIDEO_FRAME_QUEUE_SIZE, 1 }; // 视频帧队列
    double          video_frame_timer = 0.0; // 记录最后一帧视频播放的时刻
    SwsContext     *video_sws_ctx = nullptr;
    uint8_t        *video_data[4] = { nullptr };
    int             video_linesize[4] = { 0 };

    int             subtitle_index = -1;          // 字幕流索引
    AVCodecContext *subtitle_codec_ctx = nullptr; // 字幕流解码器上下文
    AVStream       *subtitle_stream = nullptr;    // 字幕流
    PacketQueue     subtitle_packet_queue;        // 字幕packet队列
    FrameQueue      subtitle_frame_queue{ &subtitle_packet_queue , SUBTITLE_FRAME_QUEUE_SIZE, 1 }; // 字幕帧队列

    // TODO
    // Clock           extern_clock{&extern_clock.m_serial, SYNC_TYPE_EXTERN};                 // 外部时钟

    double max_frame_duration = 0.0; // 一帧的最大间隔

    std::atomic<bool> paused = false;      // 暂停/恢复播放

    // seek操作
    std::atomic<bool> seek_req = false;    // seek操作
    int seek_flags = 0;
    int64_t seek_pos = 0;
    int64_t seek_rel = 0;

    bool muted = false; // 静音
    int audio_volume = 100; // 音量控制


    int eof = 0;
    // 强制刷新视频
    int force_refresh = 0;

    // 音视频同步
    Clock *master_clock = &audio_clock;

    double frame_last_returned_time = 0.0; // 用于记录上一帧在解码后被返回的时间戳
    double frame_last_filter_delay = 0.0;  // 用于记录上一帧通过滤镜链后的延迟时间
    int frame_drops_early = 0; // 统计被丢弃的时钟有误差的包，放入frame队列之前丢弃
    int frame_drops_late = 0;


    AVInputFormat *iformat = nullptr;

    std::mutex demux_mutex;
    std::condition_variable demux_cond;
};



#endif