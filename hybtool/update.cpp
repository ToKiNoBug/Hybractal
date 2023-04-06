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
#include <libHybfile.h>

#include <filesystem>
#include <iostream>

#include "hybtool.h"

namespace stdfs = std::filesystem;

using std::cout, std::endl;

bool run_update(const task_update &task) noexcept {
  int fail_counter = 0;

  for (const auto &src_name : task.files) {
    std::string dst_path = src_name;

    if (task.keep) {
      dst_path = dst_path.substr(0, dst_path.find_last_of('.'));
      dst_path += "_new.hybf";

      if (stdfs::exists(dst_path)) {
        fail_counter++;
        cout << fmt::format(
            "{} can not be updated, because {} already exists.\n", src_name,
            dst_path);
        continue;
      }
    }

    std::string err;

    libHybractal::hybf_archive archive =
        libHybractal::hybf_archive::load(src_name, &err);

    if (!err.empty()) {
      cout << fmt::format("Failed to load {}, detail: {}\n", src_name, err);
      fail_counter++;
      continue;
    }

    if (archive.metainfo().generation() == 1) {
      continue;
    }

    if (!archive.save(dst_path)) {
      cout << fmt::format("Failed to save {} to {}.\n", src_name, dst_path);
      fail_counter++;
      continue;
    }
  }

  return (fail_counter == 0);
}