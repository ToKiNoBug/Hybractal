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

#ifndef HYBRACTAL_HYBTOOL_HYBTOOL_H
#define HYBRACTAL_HYBTOOL_HYBTOOL_H

#include <libHybfile.h>
#include <libHybractal.h>

struct task_compute {
  libHybractal::hybf_metainfo info;
  std::string filename;
  uint16_t threads{1};
  bool save_mat_z{false};
  void override_x_span() noexcept {
    this->info.window_xy_span[0] =
        this->info.window_xy_span[1] * info.cols / info.rows;
  }

  void override_center(const std::array<uint64_t, 2> &src) noexcept;
};

bool run_compute(const task_compute &task) noexcept;

struct task_render {
  std::string json_file;
  std::string png_file;
  std::string hybf_file;
};

bool run_render(const task_render &task) noexcept;

struct task_look {
  std::string file{""};
  bool show_blocks{false};
  bool show_sequence{false};
  bool show_size{false};
  bool show_window{false};
  bool show_center_hex{false};
  bool show_maxit{false};
  std::string extract_age_compressed{""};
  std::string extract_age_decompress{""};
  std::string extract_z_compressed{""};
  std::string extract_z_decompress{""};
};

bool run_look(const task_look &task) noexcept;

#endif // HYBRACTAL_HYBTOOL_HYBTOOL_H