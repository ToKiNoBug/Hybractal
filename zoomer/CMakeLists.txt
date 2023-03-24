cmake_minimum_required(VERSION 3.20.0)
project(hybractal_zoomer VERSION 0.1.0 LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/configure_fractal_utils.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/configure_cli11.cmake)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

find_package(fractal_utils 2.1.2 COMPONENTS core_utils zoom_utils render_utils png_utils REQUIRED)

enable_language(CUDA)

add_executable(zoomer zoomer.cpp)

target_include_directories(zoomer PRIVATE ${CLI11_include_dir})

target_link_libraries(zoomer PRIVATE
    Qt6::Widgets
    Hybfile
    fractal_utils::zoom_utils
    fractal_utils::render_utils Render)

install(TARGETS zoomer DESTINATION
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

file(COPY ${CMAKE_SOURCE_DIR}/libRender/render1.json
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(default_hybf_file ${CMAKE_CURRENT_BINARY_DIR}/default.hybf)

add_custom_target(hybf-default
    ALL
    COMMAND ${CMAKE_BINARY_DIR}/hybtool/hybtool compute --rows 720 --cols 1080 --maxit 4096 --mat-z --center 0 0 --y-span 4 -o ${default_hybf_file}
    DEPENDS hybtool
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS ${default_hybf_file})

install(FILES ${default_hybf_file}
    DESTINATION bin)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_program(HYBRACTAL_QT_WINDEPLOYQT_EXE
        NAMES windeployqt
        PATHS ${CMAKE_PREFIX_PATH})

    configure_file(deploy.bat ${CMAKE_CURRENT_BINARY_DIR}/deploy.bat)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/deploy.bat
        DESTINATION bin)
endif()