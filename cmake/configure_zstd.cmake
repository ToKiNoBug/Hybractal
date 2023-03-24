find_package(zstd 1.5.2)

if(${zstd_FOUND})
    return()
endif()

include(FetchContent)

FetchContent_Declare(zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG v1.5.4
    SOURCE_SUBDIR build/cmake
    OVERRIDE_FIND_PACKAGE)

FetchContent_MakeAvailable(zstd)