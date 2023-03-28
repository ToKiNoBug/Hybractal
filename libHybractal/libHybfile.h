#ifndef HYBRACTAL_LIBHYBFILE_H
#define HYBRACTAL_LIBHYBFILE_H

#include <fractal_binfile.h>

#include <optional>

#include "libHybractal.h"

namespace libHybractal {

struct hybf_metainfo {
  uint64_t sequence_bin{::libHybractal::convert_to_bin(HYBRACTAL_SEQUENCE_STR)};
  uint64_t sequence_len{::libHybractal::static_strlen(HYBRACTAL_SEQUENCE_STR)};

  std::array<double, 2> window_center;
  std::array<double, 2> window_xy_span;
  int maxit{100};

  size_t rows{0};
  size_t cols{0};

  inline fractal_utils::center_wind<hybf_float_t> window() const noexcept {
    fractal_utils::center_wind<hybf_float_t> ret;
    ret.center[0] = libHybractal::float_type_cvt(this->window_center[0]);
    ret.center[1] = libHybractal::float_type_cvt(this->window_center[1]);
    ret.set_x_span(this->window_xy_span[0]);
    ret.set_y_span(this->window_xy_span[1]);

    return ret;
  }
  // bool have_mat_z;
};

using hybf_metainfo_old = hybf_metainfo;

struct hybf_ir_new {
  uint64_t sequence_bin;
  uint64_t sequence_len;
  std::string center_hex;
  std::array<double, 2> window_xy_span;
  size_t rows;
  size_t cols;
  int maxit;
  int16_t float_t_prec;
};

struct hybf_metainfo_new {
 public:
  uint64_t sequence_bin{::libHybractal::convert_to_bin(HYBRACTAL_SEQUENCE_STR)};
  uint64_t sequence_len{::libHybractal::static_strlen(HYBRACTAL_SEQUENCE_STR)};
  fractal_utils::center_wind<hybf_float_t> wind;
  size_t rows{0};
  size_t cols{0};
  int maxit{100};
  int16_t float_precision{HYBRACTAL_FLT_PRECISION};

 private:
  std::string chx{};

 public:
  inline const auto &window() const noexcept { return this->wind; }

  const auto &center_hex() const noexcept { return this->chx; }

  static hybf_metainfo_new parse_metainfo(const void *src, size_t bytes,
                                          std::string &err) noexcept;

  hybf_ir_new to_ir() const noexcept;
};

struct load_options {
  std::vector<uint8_t> *compressed_age{nullptr};
  std::vector<uint8_t> *compressed_mat_z{nullptr};
  fractal_utils::binfile *binfile{nullptr};
};

class hybf_archive {
 private:
  hybf_metainfo_new m_info;
  std::vector<uint16_t> data_age;
  std::vector<std::complex<hybf_store_t>> data_z;

 public:
  enum seg_id : int64_t {
    id_metainfo = 666,
    id_mat_age = 114514,
    id_mat_z = 1919810,
  };

 public:
  hybf_archive() : hybf_archive(0, 0, false) {}
  explicit hybf_archive(size_t rows, size_t cols, bool have_z);
  auto &metainfo() noexcept { return this->m_info; }
  const auto &metainfo() const noexcept { return this->m_info; }

  inline size_t rows() const noexcept { return this->m_info.rows; }

  inline size_t cols() const noexcept { return this->m_info.cols; }

  inline auto &mat_z_data() const noexcept { return this->data_z; }

  inline auto &mat_age_data() const noexcept { return this->data_age; }

  inline bool have_mat_z() const noexcept {
    return this->data_z.size() == (this->rows() * this->cols());
  }

  fractal_utils::fractal_map map_age() noexcept {
    return fractal_utils::fractal_map{this->rows(), this->cols(),
                                      sizeof(uint16_t), this->data_age.data()};
  }

  fractal_utils::fractal_map map_z() noexcept {
    if (this->have_mat_z()) {
      return fractal_utils::fractal_map{this->rows(), this->cols(),
                                        sizeof(std::complex<hybf_float_t>),
                                        this->data_z.data()};
    }

    return fractal_utils::fractal_map{0, 0, 16, nullptr};
  }

  static hybf_archive load(std::string_view filename, std::string *err,
                           const load_options &opt = load_options()) noexcept {
    std::vector<uint8_t> buffer;
    return load(filename, buffer, err, opt);
  }

  static hybf_archive load(std::string_view filename,
                           std::vector<uint8_t> &buffer, std::string *err,
                           const load_options &opt = load_options()) noexcept;

  bool save(std::string_view filename) const noexcept;
};

void compress(const void *src, size_t bytes,
              std::vector<uint8_t> &dest) noexcept;

std::vector<uint8_t> compress(const void *src, size_t bytes) noexcept;

void decompress(const void *src, size_t src_bytes,
                std::vector<uint8_t> &dest) noexcept;

}  // namespace libHybractal

#endif  // HYBRACTAL_LIBHYBFILE_H