#include <fmt/format.h>
#include <libRender.h>

#include <iostream>

int main(int argc, char** argv) {
  int fail_count = 0;

  for (int idx = 1; idx < argc; idx++) {
    auto opt = libHybractal::hsv_render_option::load_from_file(argv[idx]);

    if (!opt.has_value()) {
      std::cout << fmt::format("Failed to load {}", argv[idx]) << std::endl;
      fail_count++;
      continue;
    }
  }
  if (fail_count <= 0) {
    std::cout << "Success" << std::endl;
  } else {
    std::cout << fmt::format("{} file(s) failed.", fail_count) << std::endl;
  }

  return fail_count;
}