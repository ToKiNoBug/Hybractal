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

#include <assert.h>
#include <fmt/format.h>
#include <hex_convert.h>
#include <omp.h>

#include "float_encode.hpp"
#include "libHybractal.h"


#define HYBRACTAL_PRIVATE_MATCH_TYPE_OLD(precision, buffer, bytes)          \
  if ((bytes) == sizeof(float_by_prec_t<(precision)>)) {                    \
    return *reinterpret_cast<const float_by_prec_t<(precision)> *>(buffer); \
  }

libHybractal::float_variant_t libHybractal::hex_to_float_old(
    std::string_view hex, std::string &err) noexcept {
  err.clear();
  if (hex.starts_with("0x") || hex.starts_with("0X")) {
    return hex_to_float_new({hex.begin() + 2, hex.end()}, err);
  }

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

  HYBRACTAL_PRIVATE_MATCH_TYPE_OLD(1, buffer, size);
  HYBRACTAL_PRIVATE_MATCH_TYPE_OLD(2, buffer, size);
  HYBRACTAL_PRIVATE_MATCH_TYPE_OLD(4, buffer, size);
  HYBRACTAL_PRIVATE_MATCH_TYPE_OLD(8, buffer, size);

  err = fmt::format(
      "Invalid floating point bytes({}), the related hex string is \"{}\"",
      size, hex);
  return NAN;
}

#define HYBRACTAL_PRIVATE_MATCH_TYPE_NEW(precision, buffer, bytes)          \
  if ((bytes) == (precision * sizeof(float))) {                             \
    return decode_float<float_by_prec_t<precision>>(buffer, bytes).value(); \
  }

libHybractal::float_variant_t libHybractal::hex_to_float_new(
    std::string_view hex, std::string &err) noexcept {
  err.clear();
  if (hex.starts_with("0x") || hex.starts_with("0X")) {
    return hex_to_float_new({hex.begin() + 2, hex.end()}, err);
  }

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
  try {
    HYBRACTAL_PRIVATE_MATCH_TYPE_NEW(1, buffer, size);
    HYBRACTAL_PRIVATE_MATCH_TYPE_NEW(2, buffer, size);
    HYBRACTAL_PRIVATE_MATCH_TYPE_NEW(4, buffer, size);
    HYBRACTAL_PRIVATE_MATCH_TYPE_NEW(8, buffer, size);
  } catch (std::exception &e) {
    err = fmt::format("Failed to decode float new. Detail: {}", err);
    return NAN;
  }

  err = fmt::format(
      "Invalid floating point bytes({}), the related hex string is \"{}\"",
      size, hex);
  return NAN;
}

libHybractal::float_variant_t libHybractal::hex_to_float_by_gen(
    std::string_view hex, int8_t gen, std::string &err) noexcept {
  if (gen == 0) {
    return hex_to_float_old(hex, err);
  }

  if (gen == 1) {
    return hex_to_float_new(hex, err);
  }

  err = fmt::format("Unknown generation {}.", int(gen));
  return NAN;
}

template <typename float_t>
void compute_frame_private(const fractal_utils::center_wind<float_t> &wind_C,
                           const uint16_t maxit,
                           fractal_utils::fractal_map &map_age_u16,
                           fractal_utils::fractal_map *map_z) noexcept {
  using namespace libHybractal;
  if (map_z != nullptr) {
    assert(map_z->rows == map_age_u16.rows);
    assert(map_z->cols == map_age_u16.cols);
    assert(map_z->element_bytes == sizeof(std::complex<hybf_store_t>));
  }

  assert(map_age_u16.element_bytes == sizeof(uint16_t));

  assert(maxit <= libHybractal::maxit_max);

  const std::complex<float_t> left_top{wind_C.left_top_corner()[0],
                                       wind_C.left_top_corner()[1]};
  const float_t r_unit = -wind_C.y_span / map_age_u16.rows;
  const float_t c_unit = wind_C.x_span / map_age_u16.cols;

#pragma omp parallel for schedule(dynamic)
  for (size_t r = 0; r < map_age_u16.rows; r++) {
    const float_t imag = left_top.imag() + r * r_unit;
    for (size_t c = 0; c < map_age_u16.cols; c++) {
      const float_t real = left_top.real() + c * c_unit;
      std::complex<float_t> z{0, 0};
      const std::complex<float_t> C{real, imag};

      int age = DECLARE_HYBRACTAL_SEQUENCE(HYBRACTAL_SEQUENCE_STR)::compute_age(
          z, C, maxit);

      if (age < 0) {
        age = UINT16_MAX;
      }

      map_age_u16.at<uint16_t>(r, c) = static_cast<uint16_t>(age);

      if (map_z != nullptr) {
        if constexpr (std::is_trivial_v<float_t>) {
          map_z->at<std::complex<hybf_store_t>>(r, c).real(double(z.real()));
          map_z->at<std::complex<hybf_store_t>>(r, c).imag(double(z.imag()));
        } else {
          auto &cplx = map_z->at<std::complex<hybf_store_t>>(r, c);
          cplx.real(float_type_cvt<float_t, hybf_store_t>(z.real()));
          cplx.imag(float_type_cvt<float_t, hybf_store_t>(z.imag()));
        }
      }
    }
  }
}

void libHybractal::compute_frame_by_precision(
    const fractal_utils::wind_base &wind_C, int precision, const uint16_t maxit,
    fractal_utils::fractal_map &map_age_u16,
    fractal_utils::fractal_map *map_z) noexcept {
  switch (precision) {
    case 1:
      compute_frame_private(
          dynamic_cast<const fractal_utils::center_wind<float_by_prec_t<1>> &>(
              wind_C),
          maxit, map_age_u16, map_z);
      break;
    case 2:
      compute_frame_private(
          dynamic_cast<const fractal_utils::center_wind<float_by_prec_t<2>> &>(
              wind_C),
          maxit, map_age_u16, map_z);
      break;
    case 4:
      compute_frame_private(
          dynamic_cast<const fractal_utils::center_wind<float_by_prec_t<4>> &>(
              wind_C),
          maxit, map_age_u16, map_z);
      break;
    case 8:
      compute_frame_private(
          dynamic_cast<const fractal_utils::center_wind<float_by_prec_t<8>> &>(
              wind_C),
          maxit, map_age_u16, map_z);
      break;
    default:
      abort();
  }
}