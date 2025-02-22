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

#define MIN_VOLUME_VALUE 0
#define DEFAULT_VOLUME_VALUE 64
#define MAX_VOLUME_VALUE 128

class AudioPlayer {
    friend void callback(void* opaque, Uint8* steram, int len);
public:
    AudioPlayer(std::shared_ptr<Context> ctx);
    int open();
    int start();
    int stop();
    int close();

    void toggle_mute();         // 静音
    void update_volume(int volume); // 更新声音

    bool is_muted() const;
private:
    void run(Uint8* steram, int len);
    int get_audio_data();
private:
    std::shared_ptr<Context> m_ctx;
    SDL_AudioDeviceID m_audio_dev_id = -1;

    int m_audio_hw_buf_size = 0;
    int m_bytes_per_sec = 0;

    uint8_t* m_current_audio_buf_data = nullptr;
    int m_current_audio_buf_size = 0;
    int m_current_audio_buf_index = 0;
    int m_current_audio_clock_serial = 0;
    double m_current_audio_clock = 0.0;
    double m_last_audio_clock = 0.0;

    std::atomic<int> m_volume = DEFAULT_VOLUME_VALUE;
    std::atomic<bool> m_muted = false;
    
};

#endif