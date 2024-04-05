#include "video_player.h"
#include <spdlog/spdlog.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/time.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#define SDL_WINDOW_DEFAULT_WIDTH  (1080)
#define SDL_WINDOW_DEFAULT_HEIGHT (720)

static const struct TextureFormatEntry {
    enum AVPixelFormat format;
    int texture_fmt;
} sdl_texture_format_map[] = {
    { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
    { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
    { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
    { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
    { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
    { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
    { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
    { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
    { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
    { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
    { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
    { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
    { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
    { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
    { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
    { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
    { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
    { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
    { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
};

static void get_sdl_pix_fmt_and_blendmode(int format, Uint32& sdl_pix_fmt, SDL_BlendMode& sdl_blend_mode) {
    sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    sdl_blend_mode = SDL_BLENDMODE_NONE;
    if (format == AV_PIX_FMT_RGB32 ||
        format == AV_PIX_FMT_RGB32_1 ||
        format == AV_PIX_FMT_BGR32 ||
        format == AV_PIX_FMT_BGR32_1) {
        sdl_blend_mode = SDL_BLENDMODE_BLEND;
    }
    for (int i = 0; i < sizeof(sdl_texture_format_map) / sizeof(TextureFormatEntry); i++) {
        if (format == sdl_texture_format_map[i].format) {
            sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
            break;
        }
    }
    return;
}

static Uint32 callback(Uint32 internal, void* param) {
    if (!param) {
        return 0;
    }
    VideoPlayer* player = static_cast<VideoPlayer*>(param);
    return player->run(internal);
}

VideoPlayer::VideoPlayer(std::shared_ptr<Context> ctx)
    : m_ctx(ctx) {

}

int VideoPlayer::open() {
    m_window = SDL_CreateWindow("ffplayer",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOW_DEFAULT_WIDTH,
                                SDL_WINDOW_DEFAULT_HEIGHT,
                                SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        spdlog::error("SDL_CreateWindow failed");
        return -1;
    }
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        m_renderer = SDL_CreateRenderer(m_window, -1, 0);
    }
    if (!m_renderer) {
        spdlog::error("SDL_CreateRenderer failed");
        exit(1);
    }
    SDL_RendererInfo info;
    SDL_GetRendererInfo(m_renderer, &info);
    spdlog::info("open video player: name={}, num_texture_formats={}", info.name, info.num_texture_formats);
    return 0;
}

int VideoPlayer::start() {
    if (m_timer_id != 0) {
        spdlog::warn("this video player already started");
        return 0;
    }
    int internal = 1000.0 / av_q2d(m_ctx->video_frame_rate);
    m_timer_id = SDL_AddTimer(internal, callback, this);
    if (0 == m_timer_id) {
        spdlog::error("add video player timer failed");
        return -1;
    }
    spdlog::info("start video player, internal={}", internal);
    return 0;
}

int VideoPlayer::close() {
    SDL_RemoveTimer(m_timer_id);
    return 0;
}

void VideoPlayer::toggle_full_screen() {
    m_is_full_screen = !m_is_full_screen;
    SDL_SetWindowFullscreen(m_window, m_is_full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

int VideoPlayer::run(int internal) {
    double remaining_time = refresh();
    internal = (1000.0 / av_q2d(m_ctx->video_frame_rate)) / 2.0;
    return internal;
}

double VideoPlayer::refresh() {
    double remaining_time = 0.0;
    if (m_ctx->video_stream) {
retry:
        if (m_ctx->video_frame_queue.nb_remaining() > 0) {
            Frame *lastvp = m_ctx->video_frame_queue.peek_last(); // 上一帧：上次已显示的帧
            Frame *vp = m_ctx->video_frame_queue.peek(); //// 当前帧：当前待显示的帧
            // 快进和快退的时候会导致缓存的帧失效，所以这里丢弃缓存帧
            if (vp->serial != m_ctx->video_packet_queue.serial()) {
                m_ctx->video_frame_queue.next();
                goto retry;
            }
            // lastvp和vp不是同一播放序列(一个seek会开始一个新播放序列)，将frame_timer更新为当前时间
            if (lastvp->serial != vp->serial) {
                m_ctx->video_frame_timer = av_gettime_relative() / 1000000.0;
            }
            // 暂停处理：不停播放上一帧图像
            if (m_ctx->paused) {
                goto display;
            }
            // 计算当前帧需要播放的delay时长
            double last_duration = lastvp->vf_duration(vp, m_ctx->max_frame_duration);
            double delay = compute_target_delay(last_duration);

            // 当前时间
            double time = av_gettime_relative() / 1000000.0;
            // 当前帧播放时刻(is->frame_timer+delay)大于当前时刻(time)，表示播放时刻未到
            if (time < m_ctx->video_frame_timer + delay) {
                // 播放时刻未到，则更新刷新时间remaining_time为当前时刻到下一播放时刻的时间差
                remaining_time = FFMIN(m_ctx->video_frame_timer + delay - time, remaining_time);
                // 播放时刻未到，则不更新rindex，把上一帧再lastvp再播放一遍
                goto display;
            }
            // 更新frame_timer值
            m_ctx->video_frame_timer += delay;
            // 校正frame_timer值：若frame_timer落后于当前系统时间太久(超过最大同步域值)，则更新为当前系统时间
            if (delay > 0 && time - m_ctx->video_frame_timer > AV_SYNC_THRESHOLD_MAX) {
                m_ctx->video_frame_timer = time;
            }
            // 更新视频时钟：时间戳、时钟时间
            m_ctx->video_frame_queue.lock();
            if (!isnan(vp->pts)) {
                update_video_pts(vp->pts, vp->serial);
            }
            m_ctx->video_frame_queue.unlock();

            // 是否要丢弃未能及时播放的视频帧
            if (m_ctx->video_frame_queue.nb_remaining() > 1) { // 队列中未显示帧数>1(只有一帧则不考虑丢帧)
                Frame* nextvp = m_ctx->video_frame_queue.peek_next(); // 下一帧：下一待显示的帧
                double duration = vp->vf_duration(nextvp, m_ctx->max_frame_duration); // 当前帧vp播放时长 = nextvp->pts - vp->pts
                // 1. 非步进模式；2. 丢帧策略生效；3. 当前帧vp未能及时播放，即下一帧播放时刻(is->frame_timer+duration)小于当前系统时刻(time)
                // TODO 步进播放模式
                if (/*!m_ctx->step &&*/ (framedrop > 0 || (framedrop && m_ctx->master_clock->sync_type() != SYNC_TYPE_VIDEO)) && time > m_ctx->video_frame_timer + duration) {
                    m_ctx->frame_drops_late++; // framedrop丢帧处理有两处：1) packet入队列前，2) frame未及时显示(此处)
                    m_ctx->video_frame_queue.next(); // 删除上一帧已显示帧，即删除lastvp，读指针加1(从lastvp更新到vp)
                    goto retry;
                }
            }
            // 删除当前读指针元素，读指针+1。
            // 若未丢帧，读指针从lastvp更新到vp；
            // 若有丢帧，读指针从vp更新到nextvp
            m_ctx->video_frame_queue.next();
            m_ctx->force_refresh = 1;

            // TODO 步进播放
            // if (is->step && !is->paused)
            //  stream_toggle_pause(is);
        }
    display:
        if (/*!display_disable && is->show_mode == SHOW_MODE_VIDEO */ m_ctx->force_refresh && m_ctx->video_frame_queue.rindex_shown()) {
            display();
        }
        m_ctx->force_refresh = 0;
    }
    return remaining_time;
}

void VideoPlayer::display() {
    int ret = 0;
    ret = SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);

    ret = SDL_RenderClear(m_renderer);
    render();
    SDL_RenderPresent(m_renderer);
    if (ret != 0) {
        spdlog::error("ret={}, error={}", ret, SDL_GetError());
    }
}

void VideoPlayer::render() {
    Frame* vp = m_ctx->video_frame_queue.peek_last();
    if (!vp || vp->uploaded) {
        spdlog::error("vp is nullptr");
        return;
    }

    if (!m_ctx->video_sws_ctx) {
        m_ctx->video_sws_ctx = sws_getContext(
            vp->frame->width, vp->frame->height, static_cast<AVPixelFormat>(vp->frame->format),
            SDL_WINDOW_DEFAULT_WIDTH, SDL_WINDOW_DEFAULT_HEIGHT, static_cast<AVPixelFormat>(vp->frame->format),
            SWS_BILINEAR, NULL, NULL, NULL);
        if (!m_ctx->video_sws_ctx) {
            spdlog::error("sws_getContext failed");
            return;
        }
        int ret = av_image_alloc(m_ctx->video_data, m_ctx->video_linesize,
            SDL_WINDOW_DEFAULT_WIDTH, SDL_WINDOW_DEFAULT_HEIGHT, static_cast<AVPixelFormat>(vp->frame->format), 1);
        if (ret < 0) {
            spdlog::error("av_image_alloc failed");
            return;
        }
    }

    sws_scale(m_ctx->video_sws_ctx, (const uint8_t* const*)vp->frame->data, vp->frame->linesize, 0,
        vp->frame->height, m_ctx->video_data, m_ctx->video_linesize);

    int ret = 0;
    Uint32 sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    SDL_BlendMode sdl_blendmode = SDL_BLENDMODE_NONE;
    get_sdl_pix_fmt_and_blendmode(vp->frame->format, sdl_pix_fmt, sdl_blendmode);
    create_texture(sdl_pix_fmt, SDL_WINDOW_DEFAULT_WIDTH, SDL_WINDOW_DEFAULT_HEIGHT, sdl_blendmode);
    switch (sdl_pix_fmt) {
    case SDL_PIXELFORMAT_IYUV:
        if (m_ctx->video_linesize[0] > 0 &&
            m_ctx->video_linesize[1] > 0 &&
            m_ctx->video_linesize[2] > 0) {
            ret = SDL_UpdateYUVTexture(m_texture, nullptr,
                  m_ctx->video_data[0], m_ctx->video_linesize[0],
                  m_ctx->video_data[1], m_ctx->video_linesize[1],
                  m_ctx->video_data[2], m_ctx->video_linesize[2]);
        }
        else if (m_ctx->video_linesize[0] < 0 && 
                    m_ctx->video_linesize[1] < 0 && 
                    m_ctx->video_linesize[2] < 0) {
            ret = SDL_UpdateYUVTexture(m_texture, nullptr, 
                  m_ctx->video_data[0] + m_ctx->video_linesize[0] * (SDL_WINDOW_DEFAULT_HEIGHT - 1), -m_ctx->video_linesize[0],
                  m_ctx->video_data[1] + m_ctx->video_linesize[1] * (AV_CEIL_RSHIFT(SDL_WINDOW_DEFAULT_HEIGHT, 1) - 1), -m_ctx->video_linesize[1],
                  m_ctx->video_data[2] + m_ctx->video_linesize[2] * (AV_CEIL_RSHIFT(SDL_WINDOW_DEFAULT_HEIGHT, 1) - 1), -m_ctx->video_linesize[2]);
        }
        else {
            spdlog::error("Mixed negative and positive linesizes are not supported");
        }
        break;
    default:
        if (m_ctx->video_linesize[0] < 0) {
            ret = SDL_UpdateTexture(m_texture, nullptr,
                    m_ctx->video_data[0] + m_ctx->video_linesize[0] * (SDL_WINDOW_DEFAULT_HEIGHT - 1), -m_ctx->video_linesize[0]);
        }
        else {
            ret = SDL_UpdateTexture(m_texture, nullptr, m_ctx->video_data[0], m_ctx->video_linesize[0]);
        }
        break;
    }
    if (ret != 0) {
        spdlog::error("ret={}, error={}", ret, SDL_GetError());
    }
    vp->uploaded = 1;

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = SDL_WINDOW_DEFAULT_WIDTH;
    rect.h = SDL_WINDOW_DEFAULT_HEIGHT;
    ret = SDL_RenderCopy(m_renderer, m_texture, nullptr, &rect);
    if (ret != 0) {
        spdlog::error("ret={}, error={}", ret, SDL_GetError());
    }
}

void VideoPlayer::create_texture(Uint32 format, int width, int height, SDL_BlendMode blendmode) {
    bool recreate = true;
    Uint32 fmt;
    int access, w, h; fmt;
    if (m_texture &&
        SDL_QueryTexture(m_texture, &fmt, &access, &w, &h) >= 0
        && fmt == format && w == width && h == height) {
        return;
    }
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
    }
    m_texture = SDL_CreateTexture(m_renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!m_texture) {
        spdlog::error("SDL_CreateTexture failed");
        return;
    }
    if (SDL_SetTextureBlendMode(m_texture, blendmode) < 0) {
        spdlog::error("SDL_SetTextureBlendMode failed");
        return;
    }
    spdlog::info("create texture, width={}, height={}, format={}", width, height, SDL_GetPixelFormatName(format));
}

// 根据视频时钟与同步时钟(如音频时钟)的差值，校正delay值，使视频时钟追赶或等待同步时钟
// 输入参数delay是上一帧播放时长，即上一帧播放后应延时多长时间后再播放当前帧，通过调节此值来调节当前帧播放快慢
// 返回值delay是将输入参数delay经校正后得到的值
double VideoPlayer::compute_target_delay(double delay) {
    double delay0 = delay;
    if (m_ctx->master_clock->sync_type() != SYNC_TYPE_VIDEO) {
        // 视频时钟与同步时钟(如音频时钟)的差异，时钟值是上一帧pts值(实为：上一帧pts + 上一帧至今流逝的时间差)
        double diff = m_ctx->video_clock.get() - m_ctx->master_clock->get(); 

        // delay是上一帧播放时长：当前帧(待播放的帧)播放时间与上一帧播放时间差理论值
        // diff是视频时钟与同步时钟的差值

        // 若delay < AV_SYNC_THRESHOLD_MIN，则同步域值为AV_SYNC_THRESHOLD_MIN
        // 若delay > AV_SYNC_THRESHOLD_MAX，则同步域值为AV_SYNC_THRESHOLD_MAX
        // 若AV_SYNC_THRESHOLD_MIN < delay < AV_SYNC_THRESHOLD_MAX，则同步域值为delay
        double sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));

        if (!isnan(diff) && fabs(diff) < m_ctx->max_frame_duration) {
            if (diff <= -sync_threshold) { // 视频时钟落后于同步时钟，且超过同步域值
                delay = FFMAX(0, delay + diff); // 当前帧播放时刻落后于同步时钟(delay+diff<0)则delay=0(视频追赶，立即播放)，否则delay=delay+diff
            }
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) { // 视频时钟超前于同步时钟，且超过同步域值，但上一帧播放时长超长
                delay = delay + diff; // 仅仅校正为delay=delay+diff，主要是AV_SYNC_FRAMEDUP_THRESHOLD参数的作用，不作同步补偿
            }
            else if (diff >= sync_threshold) { // 视频时钟超前于同步时钟，且超过同步域值
                delay = 2 * delay; // 视频播放要放慢脚步，delay扩大至2倍
            }
        }
        // spdlog::info("video: delay={:.3f}, delay0={:.3f}, A-V={:.3f}", delay, delay0, diff);
    };
    return delay;
}

void VideoPlayer::update_video_pts(double pts, int serial) {
    m_ctx->video_clock.set(pts, serial);
    // m_ctx->video_clock.sync_from_slave(m_ctx->extern_clock);
}