cmake_minimum_required(VERSION 3.20.0)
project(libHybractal VERSION 0.1.0 LANGUAGES CXX)

add_library(Hybractal STATIC libHybractal.h libHybractal.cpp)

target_compile_features(Hybractal PUBLIC cxx_std_20)
target_include_directories(Hybractal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(fractal_utils COMPONENTS core_utils REQUIRED)
find_package(OpenMP REQUIRED)

target_link_libraries(Hybractal PUBLIC fractal_utils::core_utils OpenMP::OpenMP_CXX)

find_package(Protobuf REQUIRED)

# tests
add_executable(test_Hybractal test.cpp)
target_link_libraries(test_Hybractal PUBLIC Hybractal)