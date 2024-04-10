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

#define SDL_WINDOW_DEFAULT_WIDTH  (1080)
#define SDL_WINDOW_DEFAULT_HEIGHT (720)

class VideoPlayer {
    friend Uint32 callback(Uint32 internal, void* param);
public:
    VideoPlayer(std::shared_ptr<Context> ctx);
    int open();
    int start();
    int close();

    void update_width_height(int width, int height);
    void toggle_full_screen();

    int run(int interval);
private:
    
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

    SwsContext* m_sws_ctx = nullptr;
    uint8_t* m_data[4] = { nullptr };
    int m_linesize[4] = { 0 };
    int m_data_size = 0;

    int m_dst_width = SDL_WINDOW_DEFAULT_WIDTH;
    int m_dst_height = SDL_WINDOW_DEFAULT_HEIGHT;
    int m_width = SDL_WINDOW_DEFAULT_WIDTH;
    int m_height = SDL_WINDOW_DEFAULT_HEIGHT;
    bool m_is_full_screen = false;
};

#endif