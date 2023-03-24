cmake_minimum_required(VERSION 3.20.0)
project(hybtool VERSION 0.1.0 LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/configure_cli11.cmake)

enable_language(CUDA)

find_package(OpenMP REQUIRED)
find_package(fractal_utils COMPONENTS png_utils REQUIRED)

add_executable(hybtool
    hybtool.h
    hybtool.cpp
    compute.cpp
    render.cpp
    look.cpp)

target_link_libraries(hybtool PRIVATE Hybractal Hybfile Render OpenMP::OpenMP_CXX fractal_utils::png_utils)
target_include_directories(hybtool PRIVATE ${CLI11_include_dir})

install(TARGETS hybtool
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)