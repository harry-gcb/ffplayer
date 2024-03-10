cmake_minimum_required(VERSION 3.20)

project(ffplayer)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

find_package(FFMPEG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(ffplayer
    main.cpp
    event_loop.cpp
    player.cpp
    demuxer.cpp
    decoder.cpp
    video_decoder.cpp
    audio_decoder.cpp
    packet_queue.cpp
    context.cpp
    thread_base.cpp
    opts.cpp
)

# target_include_directories(ffplayer PRIVATE ${FFMPEG_INCLUDE_DIRS})
# target_link_directories(ffplayer PRIVATE ${FFMPEG_LIBRARY_DIRS})   

target_link_libraries(ffplayer
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    ${FFMPEG_LIBRARIES}
    spdlog::spdlog
)