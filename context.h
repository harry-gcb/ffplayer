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

#ifdef __cplusplus
}
#endif

#include "packet_queue.h"

#define MAX_QUEUE_SIZE (16*1024*1024)
#define MIN_FRAMES 10000

class Context {
    friend class Player;
    friend class Demuxer;
    friend class Decoder;
    friend class AudioDecoder;
    friend class VideoDecoder;
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

    int             video_index = -1;          // 视频流索引
    AVCodecContext *video_codec_ctx = nullptr; // 视频流解码器上下文
    AVStream       *video_stream = nullptr;    // 视频流
    PacketQueue     video_packet_queue;        // 视频packet队列

    int             subtitle_index = -1;          // 字幕流索引
    AVCodecContext *subtitle_codec_ctx = nullptr; // 字幕流解码器上下文
    AVStream       *subtitle_stream = nullptr;    // 字幕流
    PacketQueue     subtitle_packet_queue;        // 字幕packet队列

    std::atomic<bool> paused = false;      // 暂停/恢复播放

    std::atomic<bool> seek_req = false;    // seek操作
    int seek_flags = 0;
    int64_t seek_pos = 0;
    int64_t seek_rel = 0;

    int eof = 0;

    AVInputFormat *iformat = nullptr;

    std::mutex demux_mutex;
    std::condition_variable demux_cond;
};



#endif