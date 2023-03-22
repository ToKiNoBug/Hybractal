cmake_minimum_required(VERSION 3.20.0)
project(libHybractal VERSION 0.1.0 LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/configure_fractal_utils.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_fmtlib.cmake)

# libHybractal
add_library(Hybractal STATIC libHybractal.h libHybractal.cpp)
target_compile_features(Hybractal PUBLIC cxx_std_20)
target_include_directories(Hybractal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
find_package(fractal_utils COMPONENTS core_utils REQUIRED)
find_package(OpenMP REQUIRED)
target_link_libraries(Hybractal PUBLIC fractal_utils::core_utils OpenMP::OpenMP_CXX)

# file
include(${CMAKE_SOURCE_DIR}/cmake/configure_yalantinglibs.cmake)
find_package(yalantinglibs REQUIRED)
find_package(LZ4 REQUIRED)
find_package(fmtlib REQUIRED)
add_library(Hybfile STATIC libHybfile.h libHybfile.cpp)
target_compile_features(Hybfile PUBLIC cxx_std_20)
target_include_directories(Hybfile INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Hybfile PUBLIC Hybractal yalantinglibs::struct_pack LZ4::lz4_shared fmt::fmt)

# tests
add_executable(test_compute test_compute.cpp)
target_link_libraries(test_compute PUBLIC Hybractal)

include(${CMAKE_SOURCE_DIR}/cmake/configure_cli11.cmake)
add_executable(test_export test_export.cpp)
target_link_libraries(test_export PUBLIC Hybractal Hybfile)
target_include_directories(test_export PUBLIC ${CLI11_include_dir})