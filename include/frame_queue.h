#ifndef FFPLAYER_FRAME_QUEUE_H_
#define FFPLAYER_FRAME_QUEUE_H_

#include <mutex>
#include <condition_variable>
#include "opts.h"
#include "packet_queue.h"
#include "frame.h"

class FrameQueue {
public:
    FrameQueue(PacketQueue *pktq, int max_size, int keep_last);
    ~FrameQueue();
    Frame *peek_writable();
    void push();
    Frame *peek_readable();
    void next();

    Frame *peek_last();
    Frame *peek();
    Frame *peek_next();

    int nb_remaining();
    int rindex_shown();
    int64_t last_pos();

    void lock();
    void unlock();
    void wakeup();
private:
    Frame m_queue[FRAME_QUEUE_SIZE] = { 0 };
    PacketQueue *m_pktq = nullptr; // 指向对应的packet_queue
    int m_max_size = FRAME_QUEUE_SIZE;
    int m_keep_last = 0;    // 是否保留已播放的最后一帧使能标志
    int m_rindex_shown = 0; // 用于跟踪最近一次显示的帧的索引, 是否保留已播放的最后一帧实现手段
    int m_size = 0;         // 队列中的总帧数
    int m_rindex = 0;       // 读索引。待播放时读取此帧进行播放，播放后此帧成为上一帧
    int m_windex = 0;       // 写索引

    std::mutex m_mutex;
    std::condition_variable m_cond;
};

#endif // FFPLAYER_FRAME_QUEUE_H_