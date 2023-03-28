cmake_minimum_required(VERSION 3.20.0)
project(
  libHybractal
  VERSION 0.1.0
  LANGUAGES CXX)

# libHybractal
add_library(Hybractal STATIC libHybractal.h libHybractal.cpp)
target_compile_features(Hybractal PUBLIC cxx_std_20)
target_include_directories(Hybractal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
find_package(
  fractal_utils
  ${HYB_fractal_utils_ver}
  COMPONENTS core_utils render_utils
  REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost)
target_link_libraries(Hybractal PUBLIC
  fractal_utils::core_utils
  OpenMP::OpenMP_CXX
  Boost::multiprecision
  fmt::fmt)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Hybractal PRIVATE "-Ofast")
  endif()

  if(${MSVC})
    target_compile_options(Hybractal PRIVATE "/fp:fast")
  endif()
endif()

# file
find_package(zstd 1.5.2 REQUIRED)
find_package(fmtlib REQUIRED)
find_package(yalantinglibs REQUIRED)

add_library(Hybfile STATIC libHybfile.h libHybfile.cpp)
target_compile_features(Hybfile PUBLIC cxx_std_20)
target_include_directories(Hybfile INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Hybfile PUBLIC
  Hybractal
  yalantinglibs::struct_pack
  zstd::libzstd_static
  fmt::fmt)

# tests
add_executable(test_compute test_compute.cpp)
target_link_libraries(test_compute PRIVATE Hybractal)

add_executable(test_export test_export.cpp)
target_link_libraries(test_export PRIVATE Hybractal Hybfile)
target_include_directories(test_export PRIVATE ${CLI11_include_dir})

add_executable(test_load test_load.cpp)
target_link_libraries(test_load PRIVATE Hybractal Hybfile fmt::fmt)
target_include_directories(test_load PRIVATE ${CLI11_include_dir})

add_executable(test_floats test_floats.cpp)
target_link_libraries(test_floats PRIVATE Boost::multiprecision Hybractal)

install(TARGETS Hybractal Hybfile
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)