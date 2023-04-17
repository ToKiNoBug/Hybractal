#https://github.com/jtravs/cuda_complex

set(cuda_complex_include_dir ${CMAKE_SOURCE_DIR}/3rdParty/cuda_complex CACHE PATH "Where to include cuda_complex")

if(EXISTS ${cuda_complex_include_dir}/cuda_complex.hpp)
    return()
endif()

execute_process(COMMAND git clone https://github.com/jtravs/cuda_complex.git
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty
    )
