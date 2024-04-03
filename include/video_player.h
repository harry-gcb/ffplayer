#ifndef FFPLAYER_VIDIO_PLAYER_H_
#define FFPLAYER_VIDIO_PLAYER_H_

#include "context.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <SDL.h>
#ifdef __cplusplus
}
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
    double refresh();
    void display();
    void render();
    void create_texture(Uint32 format, int width, int height, SDL_BlendMode blendmode);
    
    double compute_target_delay(double delay);
    void update_video_pts(double pts, int serial);
private:
    std::shared_ptr<Context> m_ctx = nullptr;
    SDL_TimerID m_timer_id = 0;

    SDL_Window *m_window = nullptr;
    SDL_Renderer *m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;
};

#endif