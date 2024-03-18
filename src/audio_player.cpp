#include "audio_player.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/time.h>

#ifdef __cplusplus
}
#endif

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
    m_audio_hw_buf_size = optained.size;
    m_bytes_per_sec = av_samples_get_buffer_size(nullptr, desired.channels, desired.freq, static_cast<AVSampleFormat>(desired.format), 1);
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

void AudioPlayer::run(Uint8* stream, int len) {
    int len1 = 0;
    int64_t audio_callback_time = av_gettime_relative();
    while (len > 0) {
        // audio buf缓存已经读完
        if (m_current_audio_buf_index >= m_current_audio_buf_size) {
            if (get_audio_data() < 0) {
                continue;
            }
            // TODO is->show_mode != SHOW_MODE_VIDEO
            // 设置buf大小以及读写位置
            // m_current_audio_buf_size = data.length;
            // m_current_audio_buf_index = 0;
        }
        // 复制音频数据到SDL的内存
        len1 = m_current_audio_buf_size - m_current_audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        if (!m_ctx->muted && m_current_audio_buf_data && SDL_MIX_MAXVOLUME == m_ctx->audio_volume) {
            memcpy(stream, (uint8_t*)m_current_audio_buf_data + m_current_audio_buf_index, len1);
        }
        else {
            memset(stream, 0, len1);
            if (!m_ctx->muted && m_current_audio_buf_data) {
                SDL_MixAudioFormat(stream, m_current_audio_buf_data + m_current_audio_buf_index, AUDIO_S16SYS, len1, m_ctx->audio_volume);
            }
        }
        len -= len1;
        stream += len;
        m_current_audio_buf_index += len1;
    }

    int audio_write_buf_size = m_current_audio_buf_size - m_current_audio_buf_index;
    if (!isnan(m_current_audio_clock)) {
        m_ctx->audio_clock.set_at(m_current_audio_clock - (double)(2 * m_audio_hw_buf_size + audio_write_buf_size) / m_bytes_per_sec, m_current_audio_clock_serial, audio_callback_time / 1000000.0);
    }
    return;
}

int AudioPlayer::get_audio_data() {
    Frame *af = nullptr;
    do {
        af = m_ctx->audio_frame_queue.peek_readable();
        if (!af) {
            return -1;
        }
        m_ctx->audio_frame_queue.next();
    } while (af->serial != m_ctx->audio_packet_queue.serial());

    int data_size = av_samples_get_buffer_size(nullptr,
        af->frame->ch_layout.nb_channels,
        af->frame->nb_samples,
        static_cast<AVSampleFormat>(af->frame->format),
        1);
    
    // TODO synchronize_audio

    // TODO af->frame->format != audio_src.fmt

    // TODO swr_ctx

    if (!isnan(af->pts)) {
        m_current_audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
    }
    else {
        m_current_audio_clock = NAN;
    }
    m_current_audio_clock_serial = af->serial;
    m_current_audio_buf_data = af->frame->data[0];
    m_current_audio_buf_size = data_size;
    m_current_audio_buf_index = 0;
    return 0;
}