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
    return loop.run();
}