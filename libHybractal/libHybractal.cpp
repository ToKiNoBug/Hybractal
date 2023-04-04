#include <assert.h>
#include <fmt/format.h>
#include <hex_convert.h>
#include <omp.h>

#include "libHybractal.h"

// using libHybractal::hybf_float_t;
// using libHybractal::hybf_store_t;

template <typename flt_t>
libHybractal::hybf_float_t convert(const void *src) noexcept {
  return libHybractal::template float_type_cvt<flt_t,
                                               libHybractal::hybf_float_t>(
      *reinterpret_cast<const flt_t *>(src));
}

#define HYBRACTAL_PRIVATE_MATCH_TYPE(precision, buffer, bytes) \
  if ((bytes) == sizeof(float_by_prec_t<(precision)>)) {       \
    return convert<float_by_prec_t<(precision)>>((buffer));    \
  }

std::variant<float_by_prec_t<1>, float_by_prec_t<2>, float_by_prec_t<4>,
             float_by_prec_t<8>>
libHybractal::hex_to_float(const char *beg, const char *end,
                           std::string &err) noexcept {
  err.clear();
  if (std::string_view{beg, end}.starts_with("0x")) {
    beg += 2;
  }
  std::string_view hex{beg, end};

  uint8_t buffer[4096];
  memset(buffer, 0, sizeof(buffer));

  auto bytes = fractal_utils::hex_2_bin(hex, buffer, sizeof(buffer));

  if (!bytes.has_value()) {
    err = fmt::format(
        "Failed to decode hex to binary. The hex string is \"{}\", length = {}",
        hex, hex.length());
    return NAN;
  }

  const size_t size = bytes.value();

  HYBRACTAL_PRIVATE_MATCH_TYPE(1, buffer, size);
  HYBRACTAL_PRIVATE_MATCH_TYPE(2, buffer, size);
  HYBRACTAL_PRIVATE_MATCH_TYPE(4, buffer, size);
  HYBRACTAL_PRIVATE_MATCH_TYPE(8, buffer, size);

  err = fmt::format(
      "Invalid floating point bytes({}), the related hex string is \"{}\"",
      size, hex);
  return NAN;
}

#define HYBRACTAL_PRIVATE_TO_HYBF_FLOAT_T(precision, variant)           \
  if (std::get_if<float_by_prec_t<(precision)>>(&variant) != nullptr) { \
    return libHybractal::float_type_cvt<float_by_prec_t<(precision)>,   \
                                        libHybractal::hybf_float_t>(    \
        std::get<float_by_prec_t<(precision)>>(variant));               \
  }

libHybractal::hybf_float_t libHybractal::any_type_to_compute_t(
    const char *beg, const char *end, std::string &err,
    bool *is_same_type) noexcept {
  err.clear();

  auto variant = hex_to_float(beg, end, err);

  if (!err.empty()) {
    return NAN;
  }

  HYBRACTAL_PRIVATE_TO_HYBF_FLOAT_T(1, variant);
  HYBRACTAL_PRIVATE_TO_HYBF_FLOAT_T(2, variant);
  HYBRACTAL_PRIVATE_TO_HYBF_FLOAT_T(4, variant);
  HYBRACTAL_PRIVATE_TO_HYBF_FLOAT_T(8, variant);

  err = "Impossible precision";

  return NAN;
}

void libHybractal::compute_frame(
    const fractal_utils::center_wind<hybf_float_t> &wind_C,
    const uint16_t maxit, fractal_utils::fractal_map &map_age_u16,
    fractal_utils::fractal_map *map_z) noexcept {
  if (map_z != nullptr) {
    assert(map_z->rows == map_age_u16.rows);
    assert(map_z->cols == map_age_u16.cols);
    assert(map_z->element_bytes == sizeof(std::complex<hybf_store_t>));
  }

  assert(map_age_u16.element_bytes == sizeof(uint16_t));

  assert(maxit <= libHybractal::maxit_max);

  const std::complex<hybf_float_t> left_top{wind_C.left_top_corner()[0],
                                            wind_C.left_top_corner()[1]};
  const hybf_float_t r_unit = -wind_C.y_span / map_age_u16.rows;
  const hybf_float_t c_unit = wind_C.x_span / map_age_u16.cols;

#pragma omp parallel for schedule(dynamic)
  for (size_t r = 0; r < map_age_u16.rows; r++) {
    const hybf_float_t imag = left_top.imag() + r * r_unit;
    for (size_t c = 0; c < map_age_u16.cols; c++) {
      const hybf_float_t real = left_top.real() + c * c_unit;
      std::complex<hybf_float_t> z{0, 0};
      const std::complex<hybf_float_t> C{real, imag};

      int age = DECLARE_HYBRACTAL_SEQUENCE(HYBRACTAL_SEQUENCE_STR)::compute_age(
          z, C, maxit);

      if (age < 0) {
        age = UINT16_MAX;
      }

      map_age_u16.at<uint16_t>(r, c) = static_cast<uint16_t>(age);

      if (map_z != nullptr) {
        if constexpr (std::is_trivial_v<hybf_float_t>) {
          map_z->at<std::complex<hybf_store_t>>(r, c).real(double(z.real()));
          map_z->at<std::complex<hybf_store_t>>(r, c).imag(double(z.imag()));
        } else {
          auto &cplx = map_z->at<std::complex<hybf_store_t>>(r, c);
          cplx.real(float_type_cvt<hybf_float_t, hybf_store_t>(z.real()));
          cplx.imag(float_type_cvt<hybf_float_t, hybf_store_t>(z.imag()));
        }
      }
    }
  }
}
