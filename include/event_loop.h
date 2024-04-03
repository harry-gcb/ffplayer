#ifndef FFPLAYER_EVENT_LOOP_H_
#define FFPLAYER_EVENT_LOOP_H_

#include <map>
#include <functional>

#include <SDL.h>

#define USER_EVENT_BASE  (SDL_USEREVENT+100)
#define USER_EVENT_TIMER (USER_EVENT_BASE+10)

class EventLoop {
public:
    EventLoop();

    int run();
    void quit();
    void addEvent(int type, const std::function<void(SDL_Event*)> &cb);

    static EventLoop *instance();
private:
    std::map<int, std::function<void(SDL_Event *)>> m_eventMap;
};

#endif