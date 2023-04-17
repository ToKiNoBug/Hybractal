#ifndef HYBRACTAL_CUBRACTAL_H
#define HYBRACTAL_CUBRACTAL_H

#include "libHybractal.h"

namespace libHybractal {
class cubractal_resource {
 private:
  size_t _rows{0};
  size_t _cols{0};
  uint16_t *gpu_age{nullptr};
  std::complex<double> *gpu_z{nullptr};

 public:
  inline size_t rows() const noexcept { return this->_rows; }
  inline size_t cols() const noexcept { return this->_cols; }
  inline size_t size() const noexcept { return this->_rows * this->_cols; }

  inline uint16_t *data_age_gpu() noexcept { return this->gpu_age; }
  inline const uint16_t *data_age_gpu() const noexcept { return this->gpu_age; }

  inline std::complex<double> *data_z_gpu() noexcept { return this->gpu_z; }
  inline const std::complex<double> *data_z_gpu() const noexcept {
    return this->gpu_z;
  }

  inline bool is_valid() const noexcept {
    return (this->_rows > 0) && (this->_cols > 0) &&
           (this->gpu_age != nullptr) && (this->gpu_z != nullptr);
  }

  cubractal_resource(size_t rows, size_t cols);
  ~cubractal_resource();

  cubractal_resource(const cubractal_resource &) = delete;
  cubractal_resource &operator=(const cubractal_resource &) = delete;

  cubractal_resource(cubractal_resource &&);
  cubractal_resource &operator=(cubractal_resource &&);
};

std::string compute_frame_cuda(const fractal_utils::wind_base &wind_C,
                               int precision, const uint16_t maxit,
                               fractal_utils::fractal_map &map_age_u16,
                               fractal_utils::fractal_map *map_z_nullable,
                               cubractal_resource &gpu_rcs) noexcept;
}  // namespace libHybractal

#endif  // HYBRACTAL_CUBRACTAL_H