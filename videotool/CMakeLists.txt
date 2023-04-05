cmake_minimum_required(VERSION 3.20.0)
project(
    Hybractal_videotool
    VERSION 0.1.0
    LANGUAGES CXX)

enable_language(CUDA)

add_executable(videotool
    videotool.h
    videotool.cpp
    load_video_task.cpp
    compute.cpp
    render.cpp
    makevideo.cpp)

target_link_libraries(videotool PRIVATE Hybractal Hybfile Render)
target_include_directories(videotool PRIVATE ${CLI11_include_dir} ${njson_include_dir})

find_package(fmtlib REQUIRED)
find_package(OpenMP REQUIRED)
target_link_libraries(videotool PRIVATE fmt OpenMP::OpenMP_CXX)

install(TARGETS videotool
    RUNTIME DESTINATION bin)

add_test(NAME videotool-parse
    COMMAND videotool ${CMAKE_CURRENT_SOURCE_DIR}/videotask.json)