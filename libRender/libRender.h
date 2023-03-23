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

#ifndef HYBRACTAL_LIBRENDER_LIBRENDER_H
#define HYBRACTAL_LIBRENDER_LIBRENDER_H

#include <fractal_colors.h>
#include <fractal_map.h>
#include <libHybractal.h>

#include <optional>

#ifdef __CUDA__
#define HYBRACTAL_HOST_DEVICE_FUN __host__ __device__
#else

#define HYBRACTAL_HOST_DEVICE_FUN
#endif //--expt-relaxed-constexpr

namespace libHybractal {

enum frac_val : uint8_t { fv_age = 0, fv_norm2 = 1, fv_angle = 2 };

struct hsv_render_option {
  struct hsv_range {
    // range : [0, 360]
    std::array<float, 2> range_H;
    // [0, 1]
    std::array<float, 2> range_S;
    // [0, 1]
    std::array<float, 2> range_V;
    float age_peroid;
    std::array<frac_val, 3> fv_mapping;

    inline std::array<float, 3>
    map_value(const std::array<float, 3> &src) const noexcept {
      std::array<float, 3> hsv;

      hsv[0] = (range_H[1] - range_H[0]) * src[fv_mapping[0]] + range_H[0];
      hsv[1] = (range_S[1] - range_S[0]) * src[fv_mapping[1]] + range_S[0];
      hsv[2] = (range_V[1] - range_V[0]) * src[fv_mapping[2]] + range_V[0];

      return hsv;
    }
  };
  hsv_range range_age_inf;
  hsv_range range_age_normal;

  static std::optional<hsv_render_option> load(const char *beg,
                                               const char *end) noexcept;
  static std::optional<hsv_render_option>
  load_from_file(std::string_view filename) noexcept;
};

class gpu_resource {
private:
  size_t m_rows;
  size_t m_cols;

  gpu_resource(const gpu_resource &) = delete;

  auto operator=(const gpu_resource &) = delete;
  auto operator=(gpu_resource &&) = delete;

  uint16_t *device_mat_age{nullptr};
  std::complex<double> *device_mat_z{nullptr};
  fractal_utils::pixel_RGB *device_mat_u8c3{nullptr};

public:
  gpu_resource(size_t rows, size_t cols);
  gpu_resource(gpu_resource &&another);
  ~gpu_resource();

  inline size_t rows() const noexcept { return this->m_rows; }
  inline size_t cols() const noexcept { return this->m_cols; }

  inline uint16_t *mat_age_gpu() noexcept { return this->device_mat_age; }
  inline std::complex<double> *mat_z_gpu() noexcept {
    return this->device_mat_z;
  }
  inline fractal_utils::pixel_RGB *mat_u8c3_gpu() noexcept {
    return this->device_mat_u8c3;
  }

  inline bool ok() const noexcept {
    return (device_mat_age != nullptr) &&
           (device_mat_z != nullptr) & (device_mat_u8c3 != nullptr);
  }
};

void render_hsv(const fractal_utils::fractal_map &mat_age,
                const fractal_utils::fractal_map &mat_z,
                fractal_utils::fractal_map &mat_u8c3,
                const hsv_render_option &opt, gpu_resource &rcs) noexcept;

} // namespace libHybractal

#endif // HYBRACTAL_LIBRENDER_LIBRENDER_H