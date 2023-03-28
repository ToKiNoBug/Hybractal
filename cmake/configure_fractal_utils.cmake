
find_package(fractal_utils ${HYB_fractal_utils_ver})

if(${fractal_utils_FOUND})
    return()
endif()

include(FetchContent)

FetchContent_Declare(fractal_utils
    GIT_REPOSITORY https://github.com/ToKiNoBug/FractalUtils.git

    GIT_TAG v${HYB_fractal_utils_ver}

    # GIT_TAG main
    OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(fractal_utils)