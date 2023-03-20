cmake_minimum_required(VERSION 3.20.0)
project(Hybractal VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
enable_testing()

add_subdirectory(libHybractal)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
