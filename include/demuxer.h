#ifndef FFPLAYER_DEMUXER_H_
#define FFPLAYER_DEMUXER_H_

#include <memory>
#include "thread_base.h"
#include "context.h"

class Demuxer: public ThreadBase {
public:
    Demuxer(std::shared_ptr<Context> ctx);
    ~Demuxer();
    int open();
    int close();
    virtual void run() override;
private:
    void demux_loop();
private:
    std::shared_ptr<Context> m_ctx;
};

#endif