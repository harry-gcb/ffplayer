#include "thread_base.h"

static void ThreadEntry (void *arg) {
    ThreadBase *thread = (ThreadBase *)arg;
    thread->run();
}

void ThreadBase::start() {
    m_thread = std::thread(&ThreadBase::run, this);
}

void ThreadBase::stop() {
    m_stop = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

