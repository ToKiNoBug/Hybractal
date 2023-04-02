cmake_minimum_required(VERSION 3.20.0)
project(
  Hybractal
  VERSION 0.0.0
  LANGUAGES CXX)

include(CTest)
enable_testing()

set(CMAKE_FIND_NO_INSTALL_PREFIX TRUE)
set(CMAKE_IGNORE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

# message(STATUS "CMAKE_CXX_COMPILER = " ${CMAKE_CXX_COMPILER})
cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH compiler_path)
cmake_path(GET compiler_path PARENT_PATH compiler_path)
list(APPEND CMAKE_PREFIX_PATH ${compiler_path})
unset(compiler_path)

set(HYB_fractal_utils_ver 2.2.1)

message(STATUS "Before include: CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
#set(HYB_INSTALL_PREFIX_archive ${CMAKE_INSTALL_PREFIX})
#set(CMAKE_INSTALL_PREFIX ${HYB_INSTALL_PREFIX_archive}/3rdParty)

include(${CMAKE_SOURCE_DIR}/cmake/configure_fractal_utils.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_cli11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_fmtlib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_zstd.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_yalantinglibs.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_nlohmann_json.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_fmtlib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_boost_mp.cmake)

#set(CMAKE_INSTALL_PREFIX ${HYB_INSTALL_PREFIX_archive})
message(STATUS "After include: CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

if(${LINUX})
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()


set(HYB_sequence "1011101" CACHE STRING "")
set(HYB_float_precision 2 CACHE STRING "The bytes of floating point number. Supported values: 1, 2, 4, 8")

add_compile_definitions("HYBRACTAL_SEQUENCE_STR=\"${HYB_sequence}\"")
add_compile_definitions(_USE_MATH_DEFINES)
add_compile_definitions("HYBRACTAL_VERSION=\"${PROJECT_VERSION}\"")
add_compile_definitions("HYBRACTAL_FLT_PRECISION=${HYB_float_precision}")

if(${MSVC})
  add_compile_definitions("_SILENCE_NONFLOATING_COMPLEX_DEPRECATION_WARNING")
endif()

add_subdirectory(libHybractal)
add_subdirectory(centerhexcvt)
add_subdirectory(libRender)
add_subdirectory(hybtool)
add_subdirectory(zoomer)
add_subdirectory(videotool)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
