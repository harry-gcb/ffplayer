#include "video_player.h"
#include <spdlog/spdlog.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/time.h>

#ifdef __cplusplus
}
#endif

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
    return 0;
}

int VideoPlayer::close() {
    return 0;
}

int VideoPlayer::run(int internal) {
    double remaining_time = refresh(remaining_time);

    internal = 1000.0 / av_q2d(m_ctx->video_frame_rate);
    return internal;
}

double VideoPlayer::refresh(double &remaining_time) {
    if (m_ctx->video_stream) {
retry:
        if (m_ctx->video_frame_queue.nb_remaining()) {
            // 所有帧已显示
        }
        else {
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
            double delay = coumpte_target_delay(last_duration);

            // 当前时间
            double time = av_gettime_relative() / 100000.0;
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
                    m_ctx->frame_drops_early++; // framedrop丢帧处理有两处：1) packet入队列前，2) frame未及时显示(此处)
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
    return 0.0;
}

void VideoPlayer::display() {
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    // SDL_RenderPresent(renderer);
}

// 根据视频时钟与同步时钟(如音频时钟)的差值，校正delay值，使视频时钟追赶或等待同步时钟
// 输入参数delay是上一帧播放时长，即上一帧播放后应延时多长时间后再播放当前帧，通过调节此值来调节当前帧播放快慢
// 返回值delay是将输入参数delay经校正后得到的值
double VideoPlayer::coumpte_target_delay(double delay) {
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
    };
    return delay;
}

void VideoPlayer::update_video_pts(double pts, int serial) {
    m_ctx->video_clock.set(pts, serial);
    // m_ctx->video_clock.sync_from_slave(m_ctx->extern_clock);
}