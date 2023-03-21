#include "libHybractal.h"
#include <assert.h>
#include <omp.h>

void compute_frame(const fractal_utils::center_wind<double> &wind_C,
                   const uint16_t maxit,
                   fractal_utils::fractal_map &map_age_u16,
                   fractal_utils::fractal_map *map_z) noexcept {
  if (map_z != nullptr) {
    assert(map_z->rows == map_age_u16.rows);
    assert(map_z->cols == map_age_u16.cols);
    assert(map_z->element_bytes == sizeof(std::complex<double>));
  }

  assert(map_age_u16.element_bytes == sizeof(uint16_t));

  assert(maxit <= libHybractal::maxit_max);

  const std::complex<double> left_top{wind_C.left_top_corner()[0],
                                      wind_C.left_top_corner()[1]};
  const double r_unit = -wind_C.y_span / map_age_u16.rows;
  const double c_unit = -wind_C.x_span / map_age_u16.cols;

#pragma omp parallel for schedule(dynamic)
  for (size_t r = 0; r < map_age_u16.rows; r++) {
    const double imag = left_top.imag() + r * r_unit;
    for (size_t c = 0; c < map_age_u16.cols; c++) {
      const double real = left_top.real() + c * c_unit;
      std::complex<double> z{0, 0};
      const std::complex<double> C{real, imag};

      int age = DECLARE_HYBRACTAL_SEQUENCE(HYBRACTAL_SEQUENCE_STR)::compute_age(
          z, C, maxit);

      if (age < 0) {
        age = UINT16_MAX;
      }

      map_age_u16.at<uint16_t>(r, c) = static_cast<uint16_t>(age);

      if (map_z != nullptr) {
        map_z->at<std::complex<double>>(r, c) = z;
      }
    }
  }
}