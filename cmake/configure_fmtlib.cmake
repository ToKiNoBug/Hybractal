include(FetchContent)

find_package(fmt)

if(${fmt_FOUND})
    return()
endif()

set(CMAKE_CXX_STANDARD 20)

FetchContent_Declare(fmtlib
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 9.1.0
    OVERRIDE_FIND_PACKAGE)

FetchContent_MakeAvailable(fmtlib)