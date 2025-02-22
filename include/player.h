#ifndef FFPLAYER_PLAYER_H_
#define FFPLAYER_PLAYER_H_

#include <string>
#include <memory>
#include "context.h"
#include "demuxer.h"
#include "decoder.h"
#include "video_player.h"
#include "audio_player.h"

#define SEEK_BY_BYTES 1

class Player {
public:
    Player();

    int open(const char *filename);
    void start();
    void close();
    
    void refresh();

    void toggle_pause();
    void toggle_mute();
    void toggle_full_screen();
    void update_width_height(int width, int height);
    void force_refresh();

    void volume_up(int volume);
    void volume_down(int volume);
    void seek_forward(double incr, bool seek_by_bytes = SEEK_BY_BYTES);
    void seek_backward(double incr, bool seek_by_bytes = SEEK_BY_BYTES);
    void speed_up(double speed);
    void speed_down(double speed);

    bool is_paused() const;
    bool is_muted() const;
private:
    std::shared_ptr<Context> m_ctx;   
    std::shared_ptr<Demuxer> m_demuxer;
    std::shared_ptr<Decoder> m_audio_decoder;
    std::shared_ptr<Decoder> m_video_decoder;
    std::shared_ptr<Decoder> m_subtitle_decoder;
    std::shared_ptr<AudioPlayer> m_audio_player;
    std::shared_ptr<VideoPlayer> m_video_player;
};

#endif