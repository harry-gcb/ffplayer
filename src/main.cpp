#include <spdlog/spdlog.h>
#include "event_loop.h"
#include "player.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        spdlog::info("usage: {} input", argv[0]);
        return -1;
    }
    
    Player player;
    player.open(argv[1]);
    player.start();

    EventLoop loop;
    loop.addEvent(USER_EVENT_TIMER, std::bind(&Player::show, player));
    loop.addEvent(SDLK_SPACE, std::bind(&Player::toggle_pause, player));
    loop.addEvent(SDLK_m, std::bind(&Player::toggle_mute, player));
    loop.addEvent(SDLK_f, std::bind(&Player::toggle_full_screen, player));
    loop.addEvent(SDL_QUIT, std::bind(&Player::close, player));
    return loop.run();
}