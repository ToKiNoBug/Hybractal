add_executable(chxcvt centerhexcvt.cpp)
target_link_libraries(chxcvt PRIVATE Hybractal)
target_include_directories(chxcvt PRIVATE ${CLI11_include_dir})

install(TARGETS chxcvt 
    RUNTIME DESTINATION bin)