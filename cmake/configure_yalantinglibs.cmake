include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

FetchContent_Declare(
  yalantinglibs
  GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
  GIT_TAG main
  OVERRIDE_FIND_PACKAGE)

FetchContent_MakeAvailable(yalantinglibs)
