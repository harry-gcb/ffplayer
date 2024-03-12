#ifndef FFPLAYER_PLAYER_H_
#define FFPLAYER_PLAYER_H_

#include <string>
#include <memory>
#include "context.h"
#include "demuxer.h"
#include "decoder.h"
#include "video_player.h"
#include "audio_player.h"


class Player {
public:
    Player();

    int open(const char *filename);

    void start();
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