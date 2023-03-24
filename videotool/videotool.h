#ifndef HYBRACTAL_VIDEOTOOL_VIDEOTOOL_H
#define HYBRACTAL_VIDEOTOOL_VIDEOTOOL_H

#include <fractal_map.h>
#include <optional>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

struct common_info {
  size_t rows;
  size_t cols;
  std::string hybf_prefix;
  std::string png_prefix;
  int maxit;
};

std::string hybf_filename(const common_info &ci, int frameidx) noexcept;
std::string png_filename(const common_info &ci, int frameidx,
                         int pngidx) noexcept;

struct compute_task {
  std::string center_hex;
  double y_span;
  double x_span{-1};
  double ratio;
  int frame_num;
  int threads;
};

struct render_task {
  int png_per_frame;
  int extra_png_num;
  std::string config_file;
  int threads;
};

struct video_task {
  common_info common;
  compute_task compute;
  render_task render;
};

std::optional<video_task> load_video_task(std::string_view filename) noexcept;

bool run_compute(const common_info &common, const compute_task &ctask) noexcept;

#endif // HYBRACTAL_VIDEOTOOL_VIDEOTOOL_H