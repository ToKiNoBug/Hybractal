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
  libHybractal::hybf_metainfo_new info;
  std::string filename;
  uint16_t threads{1};
  bool save_mat_z{false};
  bool bechmark{false};
  void override_x_span() noexcept {
    const double rows = info.rows;
    const double cols = info.cols;

    auto update_x_span = [rows, cols](auto &w) -> void {
      w.x_span = w.y_span * (cols / rows);
    };

    std::visit(update_x_span, this->info.wind);
  }

  void override_center(const std::array<uint64_t, 2> &src) noexcept;
};

bool run_compute(const task_compute &task) noexcept;

struct task_render {
  std::string json_file;
  std::string png_file;
  std::string hybf_file;
  bool bechmark{false};
};

bool run_render(const task_render &task) noexcept;

struct task_look {
  std::string file{""};
  bool show_all{false};
  bool show_blocks{false};
  bool show_sequence{false};
  bool show_size{false};
  bool show_window{false};
  bool show_center_hex{false};
  bool show_maxit{false};
  bool show_precision{false};
  bool show_generation{false};
  std::string extract_age_compressed{""};
  std::string extract_age_decompress{""};
  std::string extract_z_compressed{""};
  std::string extract_z_decompress{""};
};

bool run_look(const task_look &task) noexcept;

#endif // HYBRACTAL_HYBTOOL_HYBTOOL_H