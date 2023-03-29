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

#ifndef HYBRACTAL_LIBRENDER_LIBRENDER_INTERNNAL_H
#define HYBRACTAL_LIBRENDER_LIBRENDER_INTERNNAL_H

#include "libRender.h"
#include <cmath>
#include <cuComplex.h>
#include <cuda.h>
#include <iostream>

#define PRIVATE_HANDLE_ERROR_GPU_RCS(err_code)                                 \
  if (err_code) {                                                              \
    std::cerr << "cudaMalloc failed. err_code = " << err_code << std::endl;    \
  }

libHybractal::gpu_resource::gpu_resource(size_t _rows, size_t _cols)
    : m_rows(_rows), m_cols(_cols) {
  cudaError_t err_code;
  err_code =
      cudaMalloc(&this->device_mat_age, _rows * _cols * sizeof(uint16_t));
  PRIVATE_HANDLE_ERROR_GPU_RCS(err_code);
  err_code = cudaMalloc(&this->device_mat_z,
                        _rows * _cols *
                            sizeof(std::complex<libHybractal::hybf_store_t>));
  PRIVATE_HANDLE_ERROR_GPU_RCS(err_code);
  err_code =
      cudaMalloc(&this->device_mat_u8c3, _rows * _cols * sizeof(uint8_t[3]));
  PRIVATE_HANDLE_ERROR_GPU_RCS(err_code);
}

libHybractal::gpu_resource::gpu_resource(gpu_resource &&another) {
  this->m_rows = another.m_rows;
  this->m_cols = another.m_cols;

  this->device_mat_age = another.device_mat_age;
  this->device_mat_u8c3 = another.device_mat_u8c3;
  this->device_mat_z = another.device_mat_z;

  another.device_mat_age = nullptr;
  another.device_mat_u8c3 = nullptr;
  another.device_mat_z = nullptr;
}

libHybractal::gpu_resource::~gpu_resource() {
  if (device_mat_age != nullptr) {
    cudaFree(device_mat_age);
  }
  if (device_mat_z != nullptr) {
    cudaFree(device_mat_z);
  }
  if (device_mat_u8c3 != nullptr) {
    cudaFree(device_mat_u8c3);
  }
}

__device__ uchar3 hsv2rgb(const float3 HSV) noexcept {
  const float H = HSV.x;
  const float S = HSV.y;
  const float V = HSV.z;
  const float C = V * S;

  const int H_i = H;

  const int H_mod_60 = H_i / 60;

  const float X = C * (1 - std::abs((H_i / 60) % 2 - 1));
  const float m = V - C;

  float3 results[6];

  results[0] = {C, X, 0};
  results[1] = {X, C, 0};
  results[2] = {0, C, X};
  results[3] = {0, X, C};
  results[4] = {X, 0, C};
  results[5] = {C, 0, X};

  float3 RGB_ = results[H_mod_60];

  RGB_.x += m;
  RGB_.y += m;
  RGB_.z += m;

  uchar3 ret;

  ret.x = RGB_.x * 255;
  ret.y = RGB_.y * 255;
  ret.z = RGB_.z * 255;

  return ret;
}

struct cplx {
  float norm;
  float arg;
};

__device__ cplx complex_convert(cuFloatComplex z) {
  cplx ret;
  ret.norm = sqrtf(z.x * z.x + z.y * z.y);

  ret.arg = std::atan2(z.x, z.y);
  return ret;
}

__device__ cplx cplx_cvt_normalize(cuFloatComplex z) {
  cplx ret = complex_convert(z);

  ret.norm /= 2;
  ret.arg = (ret.arg + M_PI) / (2 * M_PI);

  ret.arg = std::min(1.0f, std::max(0.0f, ret.arg));
  return ret;
}

__device__ float normalize_age_cos(uint16_t age, const float peroid) {

  const float omega = 2 * M_PI / peroid;

  return 0.5f * (1 - std::cos(omega * age));
}

__device__ float get_float3_value(float3 val, int idx) {
  float temp[3]{val.x, val.y, val.z};
  return temp[idx];
}

__device__ float3
map_value(float3 src, const libHybractal::hsv_render_option::hsv_range &range) {
  float3 hsv;

  hsv.x = (range.range_H[1] - range.range_H[0]) *
              get_float3_value(src, range.fv_mapping[0]) +
          range.range_H[0];

  hsv.y = (range.range_S[1] - range.range_S[0]) *
              get_float3_value(src, range.fv_mapping[1]) +
          range.range_S[0];

  hsv.z = (range.range_V[1] - range.range_V[0]) *
              get_float3_value(src, range.fv_mapping[2]) +
          range.range_V[0];
  return hsv;
}

__global__ void render_custom(const uint16_t *age_ptr,
                              const cuDoubleComplex *z_ptr, uchar3 *u8c3_ptr,
                              const libHybractal::hsv_render_option opt) {
  static_assert(sizeof(uchar3) == 3, "");

  const int gidx = blockIdx.x * blockDim.x + threadIdx.x;
  const uint16_t age = age_ptr[gidx];
  const cuFloatComplex z{(float)z_ptr[gidx].x, (float)z_ptr[gidx].y};

  const bool is_normal = (age < libHybractal::maxit_max);

  const libHybractal::hsv_render_option::hsv_range &range =
      (is_normal) ? opt.range_age_normal : opt.range_age_inf;

  const auto normalized = cplx_cvt_normalize(z);

  const float age_normalized = normalize_age_cos(age, range.age_peroid);

  float3 HSV =
      map_value({age_normalized, normalized.norm, normalized.arg}, range);

  u8c3_ptr[gidx] = hsv2rgb(HSV);

  // auto ret = range.map_value({age_normalized, normalized.norm,
  // normalized.arg});
}

#define handle_error(err)                                                      \
  if (err) {                                                                   \
    std::cerr << "cuda error : " << err << std::endl;                          \
    abort();                                                                   \
  }

__host__ void
libHybractal::render_hsv(const fractal_utils::fractal_map &mat_age,
                         const fractal_utils::fractal_map &mat_z,
                         fractal_utils::fractal_map &mat_u8c3,
                         const hsv_render_option &opt,
                         gpu_resource &rcs) noexcept {
  assert(rcs.ok());

  assert(rcs.rows() == mat_age.rows);
  assert(rcs.rows() == mat_z.rows);
  assert(rcs.rows() == mat_u8c3.rows);

  assert(rcs.cols() == mat_age.cols);
  assert(rcs.cols() == mat_z.cols);
  assert(rcs.cols() == mat_u8c3.cols);

  cudaError_t err;

  err = cudaMemcpy(rcs.mat_age_gpu(), mat_age.data, mat_age.byte_count(),
                   cudaMemcpyKind::cudaMemcpyHostToDevice);
  handle_error(err);

  err = cudaMemcpy(rcs.mat_z_gpu(), mat_z.data, mat_z.byte_count(),
                   cudaMemcpyKind::cudaMemcpyHostToDevice);
  handle_error(err);

  err = cudaMemset(rcs.mat_u8c3_gpu(), 0xFF, mat_u8c3.byte_count());
  handle_error(err);

  static_assert(std::is_same_v<libHybractal::hybf_store_t, double>, "");

  static_assert(sizeof(cuDoubleComplex) ==
                    sizeof(std::complex<libHybractal::hybf_store_t>),
                "");
  static_assert(sizeof(uchar3) == sizeof(fractal_utils::pixel_RGB), "");

  const int blockdim = 64;

  render_custom<<<mat_age.element_count() / blockdim, blockdim>>>(
      rcs.mat_age_gpu(), (const cuDoubleComplex *)rcs.mat_z_gpu(),
      (uchar3 *)rcs.mat_u8c3_gpu(), opt);

  err = cudaMemcpy(mat_u8c3.data, rcs.mat_u8c3_gpu(), mat_u8c3.byte_count(),
                   cudaMemcpyKind::cudaMemcpyDeviceToHost);
  if (err) {
    std::cerr << "mat_u8c3.data = " << mat_u8c3.data
              << ", rcs.mat_u8c3_gpu() = " << rcs.mat_u8c3_gpu() << std::endl;
  }

  handle_error(err);
}

#endif // HYBRACTAL_LIBRENDER_LIBRENDER_INTERNNAL_H