#ifndef FFPLAYER_AUDIO_PLAYER_H_
#define FFPLAYER_AUDIO_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <SDL2/SDL_audio.h>
#ifdef __cplusplus
}
#endif

#include "context.h"

class AudioPlayer {
    friend void callback(void* opaque, Uint8* steram, int len);
public:
    AudioPlayer(std::shared_ptr<Context> ctx);
    int open();
    int start();
    int stop();
    int close();
private:
    void run(Uint8* steram, int len);
private:
    std::shared_ptr<Context> m_ctx;
    SDL_AudioDeviceID m_audio_dev_id = -1;
};

#endif