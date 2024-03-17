cmake_minimum_required(VERSION 3.20)

project(ffplayer)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

find_package(FFMPEG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(ffplayer
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/event_loop.cpp
    ${PROJECT_SOURCE_DIR}/src/player.cpp
    ${PROJECT_SOURCE_DIR}/src/demuxer.cpp
    ${PROJECT_SOURCE_DIR}/src/decoder.cpp
    ${PROJECT_SOURCE_DIR}/src/video_decoder.cpp
    ${PROJECT_SOURCE_DIR}/src/audio_decoder.cpp
    ${PROJECT_SOURCE_DIR}/src/video_player.cpp
    ${PROJECT_SOURCE_DIR}/src/audio_player.cpp
    ${PROJECT_SOURCE_DIR}/src/packet_queue.cpp
    ${PROJECT_SOURCE_DIR}/src/frame_queue.cpp
    ${PROJECT_SOURCE_DIR}/src/context.cpp
    ${PROJECT_SOURCE_DIR}/src/thread_base.cpp
    ${PROJECT_SOURCE_DIR}/src/opts.cpp
    ${PROJECT_SOURCE_DIR}/src/clock.cpp
    ${PROJECT_SOURCE_DIR}/src/frame.cpp
)

target_include_directories(ffplayer PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ffplayer
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    ${FFMPEG_LIBRARIES}
    spdlog::spdlog
)