#include "audio_player.h"

static void callback(void *opaque, Uint8 *steram, int len) {
    if (!opaque) {
        return;
    }
    AudioPlayer* player = static_cast<AudioPlayer*>(opaque);
    return player->run(steram, len);
}

AudioPlayer::AudioPlayer(std::shared_ptr<Context> ctx)
    : m_ctx(ctx) {

}

int AudioPlayer::open() {
    SDL_AudioSpec desired, optained;
    desired.channels = m_ctx->audio_codec_ctx->ch_layout.nb_channels;
    desired.freq = m_ctx->audio_codec_ctx->sample_rate;
    desired.format = AUDIO_S16SYS;
    desired.silence = 0;
    desired.samples =std::max(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(desired.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    desired.callback = callback;
    desired.userdata = this;
    m_audio_dev_id = SDL_OpenAudioDevice(nullptr, 0, &desired, &optained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    // TODO sdl desired audio format unsupported
    return m_audio_dev_id;
}

int AudioPlayer::start() {
    SDL_PauseAudioDevice(m_audio_dev_id, 0);
    return 0;
}

int AudioPlayer::stop() {
    SDL_PauseAudioDevice(m_audio_dev_id, 1);
    return 0;
}

int AudioPlayer::close() {
    return 0;
}

void AudioPlayer::run(Uint8* steram, int len) {
    return;
}