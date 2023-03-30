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
  int maxit;
  int frame_num;
  double ratio;
};

std::string hybf_filename(const common_info &ci, int frameidx) noexcept;
std::string png_filename(const common_info &ci, int frameidx,
                         int pngidx) noexcept;

struct compute_task {
  std::string center_hex;
  double y_span;
  double x_span{-1};
  int threads;
};

struct render_task {
  int png_per_frame;
  int extra_png_num;
  std::string config_file;
  int threads;
};

struct full_task {
  common_info common;
  compute_task compute;
  render_task render;
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

#endif  // HYBRACTAL_VIDEOTOOL_VIDEOTOOL_H