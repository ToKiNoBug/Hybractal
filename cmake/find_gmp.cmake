set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)

set(HYB_have_gmp OFF)

find_library(HYB_gmp_lib_c NAMES gmp PATHS ${CMAKE_PREFIX_PATH} NO_CACHE)

if(NOT HYB_gmp_lib_c)
  message(WARNING "gmp not found")
  return()
endif()

message(STATUS "HYB_gmp_lib_c found at ${HYB_gmp_lib_c}")

find_library(HYB_gmp_lib_cxx NAMES gmpxx PATHS ${CMAKE_PREFIX_PATH} NO_CACHE)


if(NOT HYB_gmp_lib_cxx)
  message(WARNING "gmp c++ lib not found")
  return()
endif()

message(STATUS "HYB_gmp_lib_cxx found at ${HYB_gmp_lib_cxx}")

find_file(HYB_gmp_cxx_header NAMES "gmpxx.h" PATHS ${CMAKE_PREFIX_PATH} NO_CACHE)

if(NOT HYB_gmp_cxx_header)
  message(WARNING "gmp cxx header is not found")
  return()
endif()

cmake_path(GET HYB_gmp_cxx_header PARENT_PATH HYB_gmp_include_dir)

message(STATUS "HYB_gmp_include_dir = ${HYB_gmp_include_dir}")


add_library(HYB_gmp_c INTERFACE)
target_include_directories(HYB_gmp_c INTERFACE ${HYB_gmp_include_dir})
target_link_libraries(HYB_gmp_c INTERFACE ${HYB_gmp_lib_c})

add_library(HYB_gmp_cxx INTERFACE)
target_link_libraries(HYB_gmp_cxx INTERFACE HYB_gmp_c ${HYB_gmp_lib_cxx})

set(HYB_have_gmp ON)

unset(CMAKE_FIND_LIBRARY_SUFFIXES)