#ifndef FFPLAYER_CLOCK_H_
#define FFPLAYER_CLOCK_H_

class Clock {
    friend class Context;
public:
    Clock(int *pkt_serial);
    void set(double pts, int serial);
    double get();
    int serial();
private:
    void set_at(double pts, int serial, double time);
private:
    int m_serial = 0;            // 时钟依赖packet队列的序列号
    int m_paused = 0;            // 播放/暂停操作
    double m_pts = 0.0;          // 时钟基准
    double m_pts_drift = 0.0;    // 时钟基准减去我们更新时钟的时间
    double m_speed = 1.0;        // 倍速播放
    double m_last_updated = 0.0; // 上次时钟更新的时间
    int *m_pkt_serial = nullptr; // packet队列中的序列号

};

#endif