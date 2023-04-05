cmake_minimum_required(VERSION 3.20.0)
project(hybtool VERSION 0.1.0 LANGUAGES CXX)

enable_language(CUDA)

find_package(OpenMP REQUIRED)
find_package(fractal_utils ${HYB_fractal_utils_ver} COMPONENTS png_utils REQUIRED)

add_executable(hybtool
    hybtool.h
    hybtool.cpp
    compute.cpp
    render.cpp
    look.cpp)

target_link_libraries(hybtool PRIVATE
    Hybractal
    Hybfile
    Render
    OpenMP::OpenMP_CXX
    fractal_utils::png_utils)

target_include_directories(hybtool PRIVATE ${CLI11_include_dir})

target_compile_definitions(hybtool PRIVATE "HYB_CMAKE_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\"")

file(COPY ${CMAKE_SOURCE_DIR}/libRender/render1.json
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS hybtool
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

add_test(NAME hybtool-compute-1
    COMMAND hybtool --rows 720 --cols 1080 --center 0 0 --precision 1 -o hybtool-compute-1.hybf)