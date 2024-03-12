#ifndef FFPLAYER_VIDIO_PLAYER_H_
#define FFPLAYER_VIDIO_PLAYER_H_

#include "context.h"

class VideoPlayer {
public:
    VideoPlayer(std::shared_ptr<Context> ctx);
    int open();
    int start();
    int close();
private:
    std::shared_ptr<Context> m_ctx;
};

#endif