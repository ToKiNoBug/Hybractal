cmake_minimum_required(VERSION 3.20.0)
project(
    Hybractal_videotool
    VERSION 0.1.0
    LANGUAGES CXX)

add_executable(videotool
    videotool.h
    videotool.cpp
    load_video_task.cpp)

target_link_libraries(videotool PRIVATE Hybractal Hybfile Render)
target_include_directories(videotool PRIVATE ${CLI11_include_dir} ${njson_include_dir})

find_package(fmtlib REQUIRED)
target_link_libraries(videotool PRIVATE fmt)
