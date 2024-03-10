#include "player.h"
#include "audio_decoder.h"
#include "video_decoder.h"

Player::Player()
{
}

// void Player::setFilePath(const std::string &filePath) {
//     m_filePath = filePath;
// }

int Player::open(const char *filename) {
    m_ctx = std::make_shared<Context>(filename);
    m_demuxer = std::make_shared<Demuxer>(m_ctx);
    m_audio_decoder = std::make_shared<AudioDecoder>(m_ctx);
    m_video_decoder = std::make_shared<VideoDecoder>(m_ctx);

    m_demuxer->open();
    m_audio_decoder->open();
    m_video_decoder->open();
    

    return 0;
}

void Player::start() {
    m_demuxer->start();
}