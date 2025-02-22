#include <spdlog/spdlog.h>
#include "event_loop.h"
#include "player.h"

int main(int argc, char *argv[]) {
    //if (argc < 2) {
    //    spdlog::info("usage: {} input", argv[0]);
    //    return -1;
    //}
    spdlog::set_pattern("[%H:%M:%S.%e] [%t] [%l] [%s:%#] %v");

    Player player;
    player.open("rtsp://admin:Gcb@cc.1314@192.168.0.107:554");
    player.start();

    EventLoop loop;
    return loop.run(player);
}