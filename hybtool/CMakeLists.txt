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

set(hybtool_flags_center_float --rows 720 --cols 1080 --maxit 50)

set(test_dir ${CMAKE_CURRENT_BINARY_DIR}/test)

file(MAKE_DIRECTORY ${test_dir})

add_test(NAME hybtool-compute-1
    COMMAND hybtool compute ${hybtool_flags_center_float} --center 0.1 0.2 --precision 1 -o hybtool-compute-1.hybf
    WORKING_DIRECTORY ${test_dir})

add_test(NAME hybtool-compute-2
    COMMAND hybtool compute ${hybtool_flags_center_float} --chx 0x96034F7405FFD73FC6AAA91F09CCE6BF --precision 2 -o hybtool-compute-2.hybf
    WORKING_DIRECTORY ${test_dir})

add_test(NAME hybtool-compute-4
    COMMAND hybtool compute ${hybtool_flags_center_float} --center 0.1 0.2 --precision 4 -o hybtool-compute-4.hybf
    WORKING_DIRECTORY ${test_dir})

add_test(NAME hybtool-compute-8
    COMMAND hybtool compute ${hybtool_flags_center_float} --chx 0xE41DFBF55A139952D75A2E73AC9C393967012F322F21AA1D2C2824C5CAFFFFBF7FA144C934C73A14AD77F6DFF7428EC6A185CC5BF2AE2756601E17813090FEBF -o hybtool-compute-8.hybf
    WORKING_DIRECTORY ${test_dir})

set(test_precision_list 1 2 4 8)

foreach(p ${test_precision_list})
    add_test(NAME hybtool-look-${p} 
        COMMAND hybtool look ${test_dir}/hybtool-compute-${p}.hybf --all
        WORKING_DIRECTORY ${test_dir})
    set_tests_properties(hybtool-look-1 PROPERTIES DEPENDS hybtool-compute-${p})

endforeach(p ${test_precision_list})

