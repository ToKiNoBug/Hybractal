cmake_minimum_required(VERSION 3.20.0)
project(
  Hybractal
  VERSION 0.1.0
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

message(STATUS "CMAKE_CXX_COMPILER = " ${CMAKE_CXX_COMPILER})

set(HYB_fractal_utils_ver 2.2.4)

# message(STATUS "Before include: CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
# set(HYB_INSTALL_PREFIX_archive ${CMAKE_INSTALL_PREFIX})
# set(CMAKE_INSTALL_PREFIX ${HYB_INSTALL_PREFIX_archive}/3rdParty)
include(${CMAKE_SOURCE_DIR}/cmake/configure_fractal_utils.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_cli11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_fmtlib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_zstd.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_yalantinglibs.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_nlohmann_json.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_fmtlib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_boost_mp.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/find_gmp.cmake)

# set(CMAKE_INSTALL_PREFIX ${HYB_INSTALL_PREFIX_archive})
# message(STATUS "After include: CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

if(${LINUX})
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

set(HYB_sequence "1011101" CACHE STRING "")
set(HYB_float128_backend boost CACHE STRING "The backend of 4 precision float. Possible values: boost, gcc_quadmath")
set(HYB_float256_backend boost CACHE STRING "The backend of 4 precision float. Possible values: boost")

add_compile_definitions("HYBRACTAL_SEQUENCE_STR=\"${HYB_sequence}\"")
add_compile_definitions(_USE_MATH_DEFINES)
add_compile_definitions("HYBRACTAL_VERSION=\"${PROJECT_VERSION}\"")

include(cmake/add_float128_backend_defines.cmake)
include(cmake/add_float256_backend_defines.cmake)

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

if(DEFINED HYB_float_precision)
  message(WARNING "HYB_float_precision must not be defined, but it is set to ${HYB_float_precision}")
endif()