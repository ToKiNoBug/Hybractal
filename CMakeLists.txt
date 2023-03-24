cmake_minimum_required(VERSION 3.20.0)
project(
  Hybractal
  VERSION 0.0.0
  LANGUAGES CXX)

include(CTest)
enable_testing()

include(${CMAKE_SOURCE_DIR}/cmake/configure_fractal_utils.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_cli11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_fmtlib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_zstd.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_yalantinglibs.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_nlohmann_json.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_fmtlib.cmake)

set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(HYBRACTAL_SEQUENCE_STR="1011101")
add_compile_definitions(_USE_MATH_DEFINES)
add_compile_definitions("HYBRACTAL_VERSION=\"${PROJECT_VERSION}\"")

# message(STATUS "CMAKE_CXX_COMPILER = " ${CMAKE_CXX_COMPILER})
cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH compiler_path)
cmake_path(GET compiler_path PARENT_PATH compiler_path)
list(APPEND CMAKE_PREFIX_PATH ${compiler_path})

# message(STATUS "CMAKE_PREFIX_PATH = " ${CMAKE_PREFIX_PATH})
unset(compiler_path)

add_subdirectory(libHybractal)
add_subdirectory(libRender)
add_subdirectory(hybtool)
add_subdirectory(zoomer)
add_subdirectory(videotool)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
