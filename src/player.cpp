#include "player.h"
#include "audio_decoder.h"
#include "video_decoder.h"
#include <spdlog/spdlog.h>

Player::Player()
{
    SDL_Init(SDL_INIT_EVERYTHING);
}

int Player::open(const char *filename) {
    m_ctx = std::make_shared<Context>(filename);
    m_demuxer = std::make_shared<Demuxer>(m_ctx);
    m_audio_decoder = std::make_shared<AudioDecoder>(m_ctx);
    m_video_decoder = std::make_shared<VideoDecoder>(m_ctx);
    m_audio_player = std::make_shared<AudioPlayer>(m_ctx);
    m_video_player = std::make_shared<VideoPlayer>(m_ctx);

    m_demuxer->open();
    m_audio_decoder->open();
    m_video_decoder->open();
    m_audio_player->open();
    m_video_player->open();

    return 0;
}

void Player::start() {
    m_demuxer->start();
    m_audio_decoder->start();
    m_video_decoder->start();
    m_audio_player->start();
    m_video_player->start();
}

void Player::close() {
    m_audio_player->close();
    m_video_player->close();
    m_audio_decoder->close();
    m_video_decoder->close();
    m_demuxer->close();
    SDL_Quit();
}

void Player::toggle_pause() {
    m_ctx->paused = !m_ctx->paused;
}

void Player::toggle_mute() {
    m_ctx->muted = !m_ctx->muted;
}

void Player::toggle_full_screen() {
    m_video_player->toggle_full_screen();
}

bool Player::is_paused() const {
    return m_ctx->paused;
}

bool Player::is_muted() const {
    return !m_ctx->muted;
}

void Player::show() {
    spdlog::info("frame_drops_early={}, frame_drops_late={}", m_ctx->frame_drops_early, m_ctx->frame_drops_late);
}