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

#ifndef HYBRACTAL_VIDEOTOOL_VIDEOTOOL_H
#define HYBRACTAL_VIDEOTOOL_VIDEOTOOL_H

#include <fractal_map.h>
#include <libHybfile.h>
#include <stddef.h>
#include <stdint.h>

#include <optional>
#include <string>
#include <vector>

struct common_info {
  size_t rows;
  size_t cols;
  std::string hybf_prefix;
  std::string png_prefix;
  std::string video_prefix;
  int maxit;
  int frame_num;
  double ratio;
};

std::array<int, 2> video_size(const common_info &ci) noexcept;

std::string hybf_filename(const common_info &ci, int frameidx) noexcept;
std::string png_filename(const common_info &ci, int frameidx,
                         int pngidx) noexcept;
std::string png_filename_expression(const common_info &ci,
                                    int frame_idx) noexcept;

struct compute_task {
  std::string center_hex;
  double y_span;
  double x_span{-1};
  int threads;
  int precision;
};

struct render_task {
  int png_per_frame;
  int extra_png_num;
  std::string config_file;
  int threads;
};

struct video_task {
  struct video_config {
    std::string extension;
    std::string encoder;
    std::string encoder_flags;
  };

  video_config itermediate_config;
  video_config product_config;
  std::string product_name;
  std::string ffmpeg_exe;
  int threads;
};

struct full_task {
  common_info common;
  compute_task compute;
  render_task render;
  video_task video;
};

std::optional<full_task> load_task(std::string_view filename) noexcept;

struct check_hybf_option {
  bool nocheck_sequence{false};
  bool ignore_size{false};
  libHybractal::hybf_archive *move_archive;
};

bool check_hybf(std::string_view filename, const common_info &ci,
                std::vector<uint8_t> &buffer, bool &exist,
                const check_hybf_option &opt = {}) noexcept;

bool check_hybf_size(const libHybractal::hybf_archive &,
                     const std::array<size_t, 2> &expected_size) noexcept;

bool run_compute(const common_info &common, const compute_task &ctask) noexcept;
bool run_render(const common_info &ci, const render_task &rt) noexcept;

bool run_makevideo(const common_info &ci, const render_task &rt,
                   const video_task &vt, bool dry_run) noexcept;

#endif  // HYBRACTAL_VIDEOTOOL_VIDEOTOOL_H