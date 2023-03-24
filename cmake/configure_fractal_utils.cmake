
find_package(fractal_utils 2.1.2)

if(${fractal_utils_FOUND})
    return()
endif()

include(FetchContent)

FetchContent_Declare(fractal_utils
    GIT_REPOSITORY https://github.com/ToKiNoBug/FractalUtils.git

    # GIT_TAG v2.1.2
    GIT_TAG main
    OVERRIDE_FIND_PACKAGE

    # URL https://github.com/ToKiNoBug/FractalUtils/archive/refs/tags/v2.0.1.tar.gz
)

FetchContent_MakeAvailable(fractal_utils)