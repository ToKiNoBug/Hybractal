add_compile_definitions("HYBRACTAL_FLOAT256_BACKEND=\"${HYB_float256_backend}\"")

if(${HYB_float256_backend} STREQUAL "boost")
    add_compile_definitions(HYBRACTAL_FLOAT256_BACKEND_BOOST)
    return()
endif()

message(FATAL "Invalid value for HYB_float256_backend: ${HYB_float256_backend}")