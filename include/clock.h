#ifndef FFPLAYER_CLOCK_H_
#define FFPLAYER_CLOCK_H_

typedef enum SyncType {
    SYNC_TYPE_AUDIO,
    SYNC_TYPE_VIDEO,
    SYNC_TYPE_EXTERN,
} SYNC_TYPE;

class Clock {
    friend class Context;
public:
    Clock(int *pkt_serial, SYNC_TYPE sync_type);
    void set(double pts, int serial);
    void set_at(double pts, int serial, double time);
    double get();
    int serial();
    // void sync_from_slave(Clock& slave);
    SYNC_TYPE sync_type() const;

private:
    int m_serial = 0;            // 时钟依赖packet队列的序列号
    int m_paused = 0;            // 播放/暂停操作
    double m_pts = 0.0;          // 时钟基准
    double m_pts_drift = 0.0;    // 时钟基准减去我们更新时钟的时间
    double m_speed = 1.0;        // 倍速播放
    double m_last_updated = 0.0; // 上次时钟更新的时间
    int *m_pkt_serial = nullptr; // packet队列中的序列号
    SYNC_TYPE m_sync_type = SYNC_TYPE_AUDIO; // 同步类型

};

#endif