find_package(Boost COMPONENTS multiprecision)

if(${Boost_FOUND})
    #return()
endif()

include(FetchContent)

FetchContent_Declare(boost_multiprecison
    GIT_REPOSITORY https://github.com/boostorg/multiprecision.git
    GIT_TAG Boost_1_81_0

    OVERRIDE_FIND_PACKAGE
    )

FetchContent_MakeAvailable(boost_multiprecison)
