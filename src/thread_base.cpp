#include "thread_base.h"
#include <spdlog/spdlog.h>

static void ThreadEntry (void *arg) {
    ThreadBase *thread = (ThreadBase *)arg;
    thread->run();
}

ThreadBase::ThreadBase() {
    spdlog::info("ThreadBase::ThreadBase, this={}", static_cast<void *>(this));
}

ThreadBase::~ThreadBase() {
    spdlog::info("ThreadBase::~ThreadBase, this={}", static_cast<void*>(this));
    stop();
}

void ThreadBase::start() {
    m_thread = std::thread(&ThreadBase::run, this);
}

void ThreadBase::stop() {
    bool expected = false; 
    if (m_stop.compare_exchange_weak(expected, true)) {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
}

