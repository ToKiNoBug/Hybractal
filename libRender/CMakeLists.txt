cmake_minimum_required(VERSION 3.20.0)
project(
  hybractal_zoomer
  VERSION 0.1.0
  LANGUAGES CXX CUDA)

find_package(
  fractal_utils
  2.1.2
  COMPONENTS core_utils render_utils png_utils
  REQUIRED)

enable_language(CUDA)

set(CMAKE_CUDA_FLAGS "--expt-relaxed-constexpr")

add_library(Render STATIC
  libRender.h
  libRender.cpp
  libRender.cu)

target_include_directories(Render PRIVATE ${njson_include_dir})
target_include_directories(Render INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  Render PUBLIC Hybractal fractal_utils::core_utils fractal_utils::render_utils
  fractal_utils::png_utils fmt::fmt)

add_executable(test_load_option test_load_option.cpp)
target_link_libraries(test_load_option PRIVATE Render fmt::fmt)

install(TARGETS Render DESTINATION
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)

install(FILES render1.json DESTINATION bin)