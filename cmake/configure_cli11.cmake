set(CLI11_include_dir ${CMAKE_SOURCE_DIR}/3rdParty/CLI11 CACHE PATH "Where to include CLI11.hpp")


if(EXISTS ${CLI11_include_dir}/CLI11.hpp)
    return()
endif()

file(DOWNLOAD
    https://github.com/CLIUtils/CLI11/releases/download/v2.3.2/CLI11.hpp
    ${CLI11_include_dir}/CLI11.hpp
)
