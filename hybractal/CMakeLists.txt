cmake_minimum_required(VERSION 3.20.0)
project(hybractal VERSION 0.1.0 LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/configure_cli11.cmake)

add_executable(hybractal hybractal.cpp)
target_link_libraries(hybractal PRIVATE Hybractal Hybfile)
target_include_directories(hybractal PRIVATE ${CLI11_include_dir})