cmake_minimum_required(VERSION 3.20.0)
project(
  Hybractal
  VERSION 0.1.0
  LANGUAGES CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(HYBRACTAL_SEQUENCE_STR="1011101")
add_compile_definitions(_USE_MATH_DEFINES)

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

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
