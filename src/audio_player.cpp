#include <spdlog/spdlog.h>
#include "audio_player.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/time.h>
#include <libavutil/opt.h>

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
    if (0 == m_audio_dev_id) {
        spdlog::error("SDL_OpenAudio failed {}", SDL_GetError());
    }

    // spdlog::error(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n", wanted_spec.channels, wanted_spec.freq, SDL_GetError());
    // TODO sdl desired audio format unsupported
    // m_audio_hw_buf_size = optained.size;
    m_audio_hw_buf_size = 2048;
    m_bytes_per_sec = av_samples_get_buffer_size(nullptr, desired.channels, desired.freq, AV_SAMPLE_FMT_S16, 1);
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
    stop();
    return 0;
}

void AudioPlayer::update_volume(int volume) {
    int temp = m_volume;
    temp += volume * av_q2d(AVRational{ 128, 100 });
    if (temp > MAX_VOLUME_VALUE) {
        temp = MAX_VOLUME_VALUE;
    }
    else if (temp < MIN_VOLUME_VALUE) {
        temp = MIN_VOLUME_VALUE;
    }
    m_volume = temp;
    spdlog::info("volume={}", temp);
}

void AudioPlayer::toggle_mute() {
    m_muted = !m_muted;
}

bool AudioPlayer::is_muted() const {
    return m_muted;
}

void AudioPlayer::run(Uint8* stream, int len) {
    int len1 = 0;
    int64_t audio_callback_time = av_gettime_relative();
    while (len > 0) {
        // audio buf缓存已经读完
        if (m_current_audio_buf_index >= m_current_audio_buf_size) {
            if (get_audio_data() < 0) {
                m_current_audio_buf_data = nullptr;
                m_current_audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE;
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
        if (!m_muted && m_current_audio_buf_data && SDL_MIX_MAXVOLUME == m_volume) {
            memcpy(stream, (uint8_t*)m_current_audio_buf_data + m_current_audio_buf_index, len1);
        }
        else {
            memset(stream, 0, len1);
            if (!m_muted && m_current_audio_buf_data) {
                SDL_MixAudioFormat(stream, m_current_audio_buf_data + m_current_audio_buf_index, AUDIO_S16SYS, len1, m_volume);
            }
        }
        len -= len1;
        stream += len1;
        m_current_audio_buf_index += len1;
    }

    int audio_write_buf_size = m_current_audio_buf_size - m_current_audio_buf_index;
    if (!isnan(m_current_audio_clock)) {
        // spdlog::info("pts={:.4f}, serial={}, time={:.4f}", m_current_audio_clock - (double)(2 * m_audio_hw_buf_size + audio_write_buf_size) / m_bytes_per_sec, m_current_audio_clock_serial, audio_callback_time / 1000000.0);
        m_ctx->audio_clock.set_at(m_current_audio_clock - (double)(2 * m_audio_hw_buf_size + audio_write_buf_size) / m_bytes_per_sec, m_current_audio_clock_serial, audio_callback_time / 1000000.0);
    }
    return;
}

int AudioPlayer::get_audio_data() {
    m_current_audio_buf_index = 0;
    if (m_ctx->paused) {
        return -1;
    }
    Frame *af = nullptr;
    do {
        af = m_ctx->audio_frame_queue.peek_readable();
        if (!af) {
            return -1;
        }
        m_ctx->audio_frame_queue.next();
    } while (af->serial != m_ctx->audio_packet_queue.serial());

    if (!m_ctx->audio_swr_ctx) {
        m_ctx->audio_swr_ctx = swr_alloc();
        if (!m_ctx->audio_swr_ctx) {
            spdlog::error("swr_alloc failed");
            return -1;
        }
         
        //swr_alloc_set_opts2(&m_ctx->audio_swr_ctx,
        //    &af->frame->ch_layout, AV_SAMPLE_FMT_S16, af->frame->sample_rate,
        //    &af->frame->ch_layout, static_cast<AVSampleFormat>(af->frame->format), af->frame->sample_rate,
        //    0, nullptr);
        av_opt_set_chlayout(m_ctx->audio_swr_ctx, "in_chlayout", &af->frame->ch_layout, 0);
        av_opt_set_int(m_ctx->audio_swr_ctx, "in_sample_rate", af->frame->sample_rate, 0);
        av_opt_set_sample_fmt(m_ctx->audio_swr_ctx, "in_sample_fmt", static_cast<AVSampleFormat>(af->frame->format), 0);

        av_opt_set_chlayout(m_ctx->audio_swr_ctx, "out_chlayout", &af->frame->ch_layout, 0);
        av_opt_set_int(m_ctx->audio_swr_ctx, "out_sample_rate", af->frame->sample_rate, 0);
        av_opt_set_sample_fmt(m_ctx->audio_swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

        if (swr_init(m_ctx->audio_swr_ctx) < 0) {
            spdlog::error("swr_init failed");
            return -1;
        }
    }

    if (m_ctx->audio_swr_ctx)
    {
        const uint8_t** in = (const uint8_t**)af->frame->extended_data;
        uint8_t** out = &m_current_audio_buf_data;
        int data_size = av_samples_get_buffer_size(nullptr,
            af->frame->ch_layout.nb_channels,
            af->frame->nb_samples,
            AV_SAMPLE_FMT_S16,
            0);
        av_fast_malloc(&m_current_audio_buf_data, (unsigned int *)&m_current_audio_buf_size, data_size);
        if (!m_current_audio_buf_data) {
            spdlog::error("av_fast_malloc failed");
            return -1;
        }
        int len = swr_convert(m_ctx->audio_swr_ctx, out, af->frame->nb_samples, in, af->frame->nb_samples);
        if (len < 0) {
            spdlog::error("swr_convert failed");
            return -1;
        }
        
        m_current_audio_buf_size = av_samples_get_buffer_size(nullptr, af->frame->ch_layout.nb_channels,
            len, AV_SAMPLE_FMT_S16, 1);
    } else {
        int data_size = av_samples_get_buffer_size(nullptr,
            af->frame->ch_layout.nb_channels,
            af->frame->nb_samples,
            static_cast<AVSampleFormat>(af->frame->format),
            1);
        m_current_audio_buf_data = af->frame->data[0];
        m_current_audio_buf_size = data_size;
    }


    // TODO synchronize_audio

    // TODO af->frame->format != audio_src.fmt

    // TODO swr_ctx
    double audio_clock0 = m_current_audio_clock;
    if (!isnan(af->pts)) {
        m_current_audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
    }
    else {
        m_current_audio_clock = NAN;
    }
    m_current_audio_clock_serial = af->serial;
    //spdlog::info("audio:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
    //    is->audio_filter_src.freq, is->audio_filter_src.ch_layout.nb_channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
    //    frame->sample_rate, frame->ch_layout.nb_channels, av_get_sample_fmt_name(frame->format), buf2, is->auddec.pkt_serial);

    // spdlog::info("audio: delay={:.3f} clock={:.3f} clock0={:.3f}", m_current_audio_clock - m_last_audio_clock, m_current_audio_clock, audio_clock0);
    m_last_audio_clock = m_current_audio_clock;
    return 0;
}