cmake_minimum_required(VERSION 3.20.0)
project(Hybractal VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(HYBRACTAL_SEQUENCE_STR="10101")

add_subdirectory(libHybractal)
add_subdirectory(hybractal)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
