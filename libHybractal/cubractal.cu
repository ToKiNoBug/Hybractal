
#include "cubractal.h"
#include <cuComplex.h>
#include <cuda.h>
#include <cuda_complex.hpp>
#include <exception>
#include <fmt/format.h>

libHybractal::cubractal_resource::cubractal_resource(size_t __rows,
                                                     size_t __cols)
    : _rows(__rows), _cols(__cols), gpu_age(nullptr), gpu_z(nullptr) {
  const size_t elements = this->size();
  if (elements <= 0) {
    return;
  }
  cudaError_t err;
  err = cudaMalloc(&this->gpu_age, elements * sizeof(uint16_t));
  if (err != cudaSuccess) {
    return;
  }
  err = cudaMalloc(&this->gpu_z, elements * sizeof(std::complex<double>));
  if (err != cudaSuccess) {
    return;
  }
}

libHybractal::cubractal_resource::~cubractal_resource() {
  if (this->gpu_age != nullptr) {
    cudaFree(this->gpu_age);
  }
  if (this->gpu_z != nullptr) {
    cudaFree(this->gpu_z);
  }
}

libHybractal::cubractal_resource::cubractal_resource(cubractal_resource &&src)
    : _rows(src._rows), _cols(src.cols()), gpu_age(src.gpu_age),
      gpu_z(src.gpu_z) {
  src.gpu_age = nullptr;
  src.gpu_z = nullptr;
}

libHybractal::cubractal_resource &
libHybractal::cubractal_resource::operator=(cubractal_resource &&src) {
  this->_rows = src._rows;
  this->_cols = src._cols;
  this->gpu_age = src.gpu_age;
  this->gpu_z = src.gpu_z;
  src.gpu_age = nullptr;
  src.gpu_z = nullptr;
  return *this;
}

template <typename flt_t> using cuda_complex_t = complex<flt_t>;

struct cuda_wind {
  cuda_complex_t<double> top_left;
  double r_unit;
  double c_unit;
};

template <typename flt_t>
void __global__ compute_by_block(std::array<int, 2> size_rc, cuda_wind wind,
                                 uint16_t maxit, uint16_t *age_ptr,
                                 cuDoubleComplex *z_ptr) {
  // x+ <=> c+
  // y+ <=> r+

  using cplx_t = cuda_complex_t<flt_t>;
  const int c = threadIdx.x + blockDim.x * blockIdx.x;
  const int r = threadIdx.y + blockDim.y * blockIdx.y;
  assert(r >= 0 && r < size_rc[0]);
  assert(c >= 0 && c < size_rc[1]);

  const cuda_complex_t<double> C_double =
      cuda_complex_t<double>{wind.top_left} +
      cuda_complex_t<double>{wind.c_unit * c, wind.r_unit * r};
  const cplx_t C{flt_t(C_double.real()), flt_t(C_double.imag())};

  cplx_t z{0, 0};

  int age = DECLARE_HYBRACTAL_SEQUENCE(
      HYBRACTAL_SEQUENCE_STR)::compute_age<float_t, cplx_t>(z, C, maxit);
  if (age < 0) {
    age = UINT16_MAX;
  }

  const int global_idx = r * size_rc[1] + c;
  age_ptr[global_idx] = age;
  if (z_ptr != nullptr) {
    z_ptr[global_idx] = {z.real(), z.imag()};
  }
  // cudaErrorAssert(r >= 0 && r < size_rc[0]);
}

template <typename float_t>
void compute_and_store(const std::complex<float_t> &C, std::array<int, 2> rc,
                       const uint16_t maxit,
                       fractal_utils::fractal_map &map_age_u16,
                       fractal_utils::fractal_map *map_z) noexcept {
  using namespace libHybractal;
  std::complex<float_t> z{0, 0};
  int age = DECLARE_HYBRACTAL_SEQUENCE(
      HYBRACTAL_SEQUENCE_STR)::compute_age<float_t>(z, C, maxit);

  if (age < 0) {
    age = UINT16_MAX;
  }
  const int r = rc[0];
  const int c = rc[1];
  map_age_u16.at<uint16_t>(r, c) = static_cast<uint16_t>(age);
  if (map_z != nullptr) {
    map_z->at<std::complex<hybf_store_t>>(r, c).real(double(z.real()));
    map_z->at<std::complex<hybf_store_t>>(r, c).imag(double(z.imag()));
  }
}

template <typename float_t>
void compute_rest(const fractal_utils::center_wind<float_t> &wind_C,
                  const uint16_t maxit, fractal_utils::fractal_map &map_age_u16,
                  fractal_utils::fractal_map *map_z_nullable,
                  std::array<int, 2> rest_rc_start) noexcept {

  const std::complex<float_t> left_top{wind_C.left_top_corner()[0],
                                       wind_C.left_top_corner()[1]};
  const float_t r_unit = -wind_C.y_span / map_age_u16.rows;
  const float_t c_unit = wind_C.x_span / map_age_u16.cols;

#pragma omp parallel for schedule(dynamic)
  for (int r = 0; r < map_age_u16.rows; r++) {
    for (int c = rest_rc_start[1]; c < map_age_u16.cols; c++) {
      const float_t real = left_top.real() + c * c_unit;
      const float_t imag = left_top.imag() + r * r_unit;

      compute_and_store<float_t>({real, imag}, {r, c}, maxit, map_age_u16,
                                 map_z_nullable);
    }
  }

#pragma omp parallel for schedule(dynamic)
  for (int c = 0; c < rest_rc_start[1]; c++) {
    for (int r = rest_rc_start[0]; r < map_age_u16.rows; r++) {
      const float_t real = left_top.real() + c * c_unit;
      const float_t imag = left_top.imag() + r * r_unit;

      compute_and_store<float_t>({real, imag}, {r, c}, maxit, map_age_u16,
                                 map_z_nullable);
    }
  }
}

std::string
libHybractal::compute_frame_cuda(const fractal_utils::wind_base &wind_C,
                                 int precision, const uint16_t maxit,
                                 fractal_utils::fractal_map &map_age_u16,
                                 fractal_utils::fractal_map *map_z_nullable,
                                 cubractal_resource &gpu_rcs) noexcept {

  if (precision != 1 && precision != 2) {
    return fmt::format(
        "Invalid precision {} for gpu. Only 1 and 2 are supported.", precision);
  }

  if (!gpu_rcs.is_valid()) {
    return fmt::format("gpu_rcs is invalid");
  }

  if (map_age_u16.rows != gpu_rcs.rows() ||
      map_age_u16.cols != gpu_rcs.cols()) {
    return fmt::format("Size mismatch. Size of map_age_u16 is [{}, {}], but "
                       "size of gpu_rcs is [{}, {}]",
                       map_age_u16.rows, map_age_u16.cols, gpu_rcs.rows(),
                       gpu_rcs.cols());
  }

  if (map_z_nullable != nullptr) {
    if (map_age_u16.rows != map_z_nullable->rows ||
        map_age_u16.cols != map_z_nullable->cols) {
      return fmt::format("Size mismatch. Size of map_age_u16 is [{}, {}], but "
                         "size of map_z is [{}, {}]",
                         map_age_u16.rows, map_age_u16.cols,
                         map_z_nullable->rows, map_z_nullable->cols);
    }
  }

  constexpr int blk_rows = 8;
  constexpr int blk_cols = 8;

  const int row_num = map_age_u16.rows / blk_rows;
  const int col_num = map_age_u16.cols / blk_cols;

  const double r_unit = -wind_C.displayed_y_span() / map_age_u16.rows;
  const double c_unit = wind_C.displayed_x_span() / map_age_u16.cols;

  cuda_complex_t<double> left_top{wind_C.displayed_left_top_corner()[0],
                                  wind_C.displayed_left_top_corner()[1]};
  std::array<int, 2> size_rc{(int)map_age_u16.rows, (int)map_age_u16.cols};
  cuDoubleComplex *const gpu_ptr_z =
      (cuDoubleComplex *)((map_z_nullable != nullptr) ? (gpu_rcs.data_z_gpu())
                                                      : (nullptr));
  if (precision == 1) {
    compute_by_block<float>
        <<<dim3(col_num, row_num), dim3(blk_cols, blk_rows)>>>(
            size_rc, {left_top, r_unit, c_unit}, maxit, gpu_rcs.data_age_gpu(),
            gpu_ptr_z);
  } else {

    compute_by_block<double>
        <<<dim3(col_num, row_num), dim3(blk_cols, blk_rows)>>>(
            size_rc, {left_top, r_unit, c_unit}, maxit, gpu_rcs.data_age_gpu(),
            gpu_ptr_z);
  }

  cudaError_t err;

  err = cudaMemcpy(map_age_u16.data, gpu_rcs.data_age_gpu(),
                   map_age_u16.byte_count(),
                   cudaMemcpyKind::cudaMemcpyDeviceToHost);
  if (err != cudaSuccess) {
    return fmt::format("cudaMemcpy failed to copy map_age with error code {}",
                       err);
  }

  if (map_z_nullable != nullptr) {

    err = cudaMemcpy(map_z_nullable->data, gpu_ptr_z,
                     map_z_nullable->byte_count(),
                     cudaMemcpyKind::cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
      return fmt::format("cudaMemcpy failed to copy map_z with error code {}",
                         err);
    }
  }

  const int rest_row_start = map_age_u16.rows - row_num * blk_rows;
  const int rest_col_start = map_age_u16.cols - col_num * blk_cols;

  if (precision == 1) {
    compute_rest<float>(
        dynamic_cast<const fractal_utils::center_wind<float> &>(wind_C), maxit,
        map_age_u16, map_z_nullable, {rest_row_start, rest_col_start});
  } else {
    compute_rest<double>(
        dynamic_cast<const fractal_utils::center_wind<double> &>(wind_C), maxit,
        map_age_u16, map_z_nullable, {rest_row_start, rest_col_start});
  }

  return {};
}