cmake_minimum_required(VERSION 3.20.0)
project(hybractal_zoomer VERSION 0.1.0 LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/configure_fractal_utils.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_cli11.cmake)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

find_package(fractal_utils COMPONENTS core_utils zoom_utils render_utils png_utils REQUIRED)

add_executable(zoomer zoomer.cpp)

target_include_directories(zoomer PRIVATE ${CLI11_include_dir})

target_link_libraries(zoomer PRIVATE
    Qt6::Widgets
    Hybfile
    fractal_utils::zoom_utils
    fractal_utils::render_utils Render)