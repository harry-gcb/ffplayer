#include "event_loop.h"
#include <spdlog/spdlog.h>

static EventLoop *gInstance = nullptr;
static const int SDL_APP_EVENT_TIMEOUT = 20;

EventLoop::EventLoop() {
    SDL_Init(SDL_INIT_EVERYTHING);
    if (!gInstance) {
        gInstance = this;
    } else {
        spdlog::error("only one instance allowed");
        exit(1);
    }
}

int EventLoop::run(Player &player) {
    SDL_Event event;
    for (;;) {
        // 带超时的事件等待，即使没有事件发生，在设置的时间到来后也会返回，对非阻塞场景比较友好
        int timeout = SDL_WaitEventTimeout(&event, 20);
        player.refresh();
        switch (event.type) {
        case SDL_QUIT: {
            player.close();
            return 0;
        }    
        break;
        case SDL_KEYDOWN: {
            switch (event.key.keysym.sym)
            {
            case SDLK_SPACE:
                player.toggle_pause();
                break;
            case SDLK_m:
                player.toggle_mute();
                break;
            case SDLK_f:
                player.toggle_full_screen();
                break;
            case SDLK_UP:
                player.volume_up(1);
                break;
            case SDLK_DOWN:
                player.volume_down(1);
                break;
            case SDLK_LEFT:
                player.seek_backward(-10.0);
                break;
            case SDLK_RIGHT:
                player.seek_forward(10.0);
                break;
            case SDLK_PAGEUP:
                player.speed_up(0.1);
                break;
            case SDLK_PAGEDOWN:
                player.speed_down(0.1);
                break;
            default:
                spdlog::info("unsupported keydown key, event.key.keysym.sym={}", event.key.keysym.sym);
                break;
            }
        }
        break;
        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                int screen_width = event.window.data1;
                int screen_height = event.window.data2;
                player.update_width_height(screen_width, screen_height);
            }
            break;
            case SDL_WINDOWEVENT_EXPOSED: {
                player.force_refresh();
            }
            break;
            default:
                // spdlog::info("unsupported window event, event.window.event={}", event.window.event);
                break;
            }
        }
        break;
        default:
            // spdlog::info("unsupported event type, event.type={}", event.type);
            break;
        }
    }
}

void EventLoop::quit() {
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

void EventLoop::addEvent(int type, const std::function<void (SDL_Event *)> &cb) {
    m_eventMap[type] = cb;
}

EventLoop *EventLoop::instance() {
    return gInstance;
}