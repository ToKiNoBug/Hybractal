/*
 Copyright © 2023  TokiNoBug
This file is part of Hybractal.

    Hybractal is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Hybractal is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Hybractal.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <fmt/format.h>
#include <libRender.h>

#include <iostream>

int main(int argc, char **argv) {
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