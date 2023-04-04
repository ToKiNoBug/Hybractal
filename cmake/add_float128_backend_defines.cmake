add_compile_definitions("HYBRACTAL_FLOAT128_BACKEND=\"${HYB_float128_backend}\"")

if(${HYB_float128_backend} STREQUAL "boost")
    add_compile_definitions(HYBRACTAL_FLOAT128_BACKEND_BOOST)
    return()
endif()

if(${HYB_float128_backend} STREQUAL "gcc_quadmath")
    add_compile_definitions(HYBRACTAL_FLOAT128_BACKEND_GCC_QUADMATH)
    return()
endif()

message(FATAL "Invalid value for HYB_float128_backend: ${HYB_float128_backend}")