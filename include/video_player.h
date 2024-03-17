#ifndef FFPLAYER_VIDIO_PLAYER_H_
#define FFPLAYER_VIDIO_PLAYER_H_

#include "context.h"
#ifdef __cplusplus

#endif
#include <SDL.h>
#ifdef __cplusplus
#endif
class VideoPlayer {
    friend Uint32 callback(Uint32 internal, void* param);
public:
    VideoPlayer(std::shared_ptr<Context> ctx);
    int open();
    int start();
    int close();
    
private:
    int run(int interval);
    double refresh(double &remaining_time);
    void display();
    
    double coumpte_target_delay(double delay);
    void update_video_pts(double pts, int serial);
private:
    std::shared_ptr<Context> m_ctx = nullptr;
    SDL_TimerID m_timer_id = 0;
};

#endif