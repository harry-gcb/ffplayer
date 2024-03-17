#include "event_loop.h"
#include <spdlog/spdlog.h>

static EventLoop *gInstance = nullptr;
static const int SDL_APP_EVENT_TIMEOUT = 10;

EventLoop::EventLoop() {
    SDL_Init(SDL_INIT_EVERYTHING);
    if (!gInstance) {
        gInstance = this;
    } else {
        spdlog::error("only one instance allowed");
        exit(1);
    }
}

int EventLoop::run() {
    SDL_Event event;
    for (;;) {
        // 带超时的事件等待，即使没有事件发生，在设置的时间到来后也会返回，对非阻塞场景比较友好
        int timeout = SDL_WaitEventTimeout(&event, SDL_APP_EVENT_TIMEOUT);
        if (0 == timeout) {
            // spdlog::info("SDL_WaitEventTimeout timeout={}", timeout);
            continue;
        }
        switch (event.type) {
            case SDL_QUIT:
                SDL_Quit();
                return 0;
            case SDL_USEREVENT:
            {
                std::function<void()> cb = *(std::function<void()>*)event.user.data1;
                cb();
            }
            break;
            default:
            {
                auto it = m_eventMap.find(event.type);
                if (it != m_eventMap.end()) {
                    auto cb = it->second;
                    cb(&event);
                }
            }
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