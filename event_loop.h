#ifndef FFPLAYER_EVENT_LOOP_H_
#define FFPLAYER_EVENT_LOOP_H_

#include <map>
#include <functional>

#include <SDL.h>

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