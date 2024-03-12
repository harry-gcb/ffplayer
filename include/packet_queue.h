#ifndef FFPLAYER_PACKET_QUEUE_H_
#define FFPLAYER_PACKET_QUEUE_H_

#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

class PacketQueue {
    struct Packet {
        AVPacket *pkt; // packet 数据
        int serial;    // 当前时序
    };
    friend class Context;
public:
    int put(AVPacket *pkt);
    int get(AVPacket *pkt, int block, int &serial);
    int count() const; // 队列中packet的个数
    int size() const;  // 队列中packet占用的内存大小
    void flush();
    void destroy();

    int  serial();
    bool request_aborted();
private:
    int _put(AVPacket *pkt);
private:
    std::queue<Packet> m_queue;
    int m_size = 0;         // 当前队列包的总大小
    int64_t m_duration = 0; // 当前队列包的总播放时长

    int m_serial = 1;
    std::atomic<bool> m_abort_request = false;

    std::mutex m_mutex;
    std::condition_variable m_cond;
};

#endif