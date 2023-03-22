include(FetchContent)

FetchContent_Declare(fractal_utils
    GIT_REPOSITORY https://github.com/ToKiNoBug/FractalUtils.git
    GIT_TAG main
    OVERRIDE_FIND_PACKAGE

    # URL https://github.com/ToKiNoBug/FractalUtils/archive/refs/tags/v2.0.1.tar.gz
)

FetchContent_MakeAvailable(fractal_utils)