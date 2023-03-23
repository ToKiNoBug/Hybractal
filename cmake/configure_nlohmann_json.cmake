set(njson_include_dir ${CMAKE_SOURCE_DIR}/3rdParty/nlohmann_json CACHE PATH "Where to include nlohmann_json")

if(EXISTS ${njson_include_dir}/nlohmann/json.hpp)
    return()
endif()

file(DOWNLOAD
https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
    ${njson_include_dir}/nlohmann/json.hpp
)
