#ifndef HYBRACTAL_LIBHYBFILE_H
#define HYBRACTAL_LIBHYBFILE_H

#include "libHybractal.h"
#include <optional>

namespace libHybractal {

struct hybf_metainfo {

  uint64_t sequence_bin{::libHybractal::convert_to_bin(HYBRACTAL_SEQUENCE_STR)};
  uint64_t sequence_len{::libHybractal::static_strlen(HYBRACTAL_SEQUENCE_STR)};

  std::array<double, 2> window_center;
  std::array<double, 2> window_xy_span;
  int maxit{100};
  // bool have_mat_z;
};

class hybf_file {
public:
  hybf_metainfo info;
  fractal_utils::fractal_map mat_age;
  std::optional<fractal_utils::fractal_map> mat_z;

public:
  explicit hybf_file();
  explicit hybf_file(size_t rows, size_t cols, bool has_mat_z);
  explicit hybf_file(hybf_metainfo &info_src,
                     fractal_utils::fractal_map &&mat_age_src,
                     std::optional<fractal_utils::fractal_map> &&mat_z_src);

  fractal_utils::center_wind<double> window() const noexcept {
    fractal_utils::center_wind<double> ret;
    ret.center = this->info.window_center;
    ret.set_x_span(this->info.window_xy_span[0]);
    ret.set_y_span(this->info.window_xy_span[1]);

    return ret;
  }

  inline bool have_z_mat() const noexcept { return this->mat_z.has_value(); }

  inline size_t rows() const noexcept { return this->mat_age.rows; }

  inline size_t cols() const noexcept { return this->mat_age.cols; }

  static hybf_file load(std::string_view filename, std::string *err) noexcept;
  static hybf_file load(std::string_view filename, std::vector<uint8_t> &buffer,
                        std::string *err) noexcept;

  bool save(std::string_view filename) const noexcept;
};

void compress(const void *src, size_t bytes,
              std::vector<uint8_t> &dest) noexcept;

std::vector<uint8_t> compress(const void *src, size_t bytes) noexcept;

void decompress(const void *src, size_t src_bytes,
                std::vector<uint8_t> &dest) noexcept;

} // namespace libHybractal

#endif // HYBRACTAL_LIBHYBFILE_H