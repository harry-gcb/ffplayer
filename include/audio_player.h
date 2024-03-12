#ifndef FFPLAYER_AUDIO_PLAYER_H_
#define FFPLAYER_AUDIO_PLAYER_H_

#include "context.h"

class AudioPlayer {
public:
    AudioPlayer(std::shared_ptr<Context> ctx);
    int open();
    int start();
    int close();
private:
    std::shared_ptr<Context> m_ctx;
};

#endif