#ifndef FFPLAYER_THREAD_BASE_H_
#define FFPLAYER_THREAD_BASE_H_

#include <thread>
#include <atomic>

class ThreadBase {
public:
    ThreadBase() = default;
    virtual ~ThreadBase() = default;

    void start();
    void stop();
    virtual void run() = 0;
protected:
    std::thread m_thread;
    std::atomic<bool> m_stop = false;
};

#endif