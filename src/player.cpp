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
    // m_video_player->start();
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
    m_ctx->m_pause_cond.notify_one();
}

void Player::toggle_mute() {
    m_audio_player->toggle_mute();
}

void Player::toggle_full_screen() {
    m_video_player->toggle_full_screen();
}

void Player::update_width_height(int width, int height) {
    m_video_player->update_width_height(width, height);
}

void Player::force_refresh() {
    m_ctx->force_refresh = 1;
}

bool Player::is_paused() const {
    return m_ctx->paused;
}

bool Player::is_muted() const {
    return m_audio_player->is_muted();
}

void Player::refresh() {
    // spdlog::info("frame_drops_early={}, frame_drops_late={}", m_ctx->frame_drops_early, m_ctx->frame_drops_late);
    m_video_player->run(0);
}

void Player::volume_up(int volume) {
    m_audio_player->update_volume(volume);
}
void Player::volume_down(int volume) {
    m_audio_player->update_volume(-volume);
}
void Player::seek_forward(double incr, bool seek_by_bytes) {
    m_demuxer->seek(incr, seek_by_bytes);
}
void Player::seek_backward(double incr, bool seek_by_bytes) {
    m_demuxer->seek(incr, seek_by_bytes);
}
void Player::speed_up(double speed) {

}
void Player::speed_down(double speed) {

}