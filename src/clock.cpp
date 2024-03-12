#include "clock.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/mathematics.h>
#include <libavutil/time.h>

#ifdef __cplusplus
}
#endif

Clock::Clock(int *pkt_serial)
    : m_speed(1.0),
      m_paused(0),
      m_pkt_serial(pkt_serial) {
    set(NAN, -1);
}

void Clock::set(double pts, int serial) {
    // 获取当前相对事件，以微秒为参数
    double time = av_gettime_relative() / 1000000.0;
    set_at(pts, serial, time);
}

void Clock::set_at(double pts, int serial, double time) {
    m_pts = pts;
    m_last_updated = time;
    m_pts_drift = m_pts - time;
    m_serial = serial;
}

double Clock::get() {
    if (*m_pkt_serial != m_serial) {
        return NAN;
    }
    if (m_paused) {
        return m_pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return m_pts_drift + time - (time - m_last_updated) * (1.0 - m_speed);
    }
}

int Clock::serial() {
    return m_serial;
}