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

#include <fmt/format.h>
#include <libHybractal.h>

#include <boost/multiprecision/cpp_bin_float.hpp>
// #include <boost/multiprecision/gmp.hpp>
#include <float_encode.hpp>
#include <iostream>

namespace bmp = boost::multiprecision;

using std::cout, std::endl;

template <int bits, int exp_bits>
using bst_floatX = bmp::number<bmp::backends::cpp_bin_float<
    bits - exp_bits, bmp::digit_base_2, void, int32_t,
    -((int64_t(1) << exp_bits) - 2), (int64_t(1) << exp_bits) - 1>>;

using bst_fl128 = boost::multiprecision::cpp_bin_float_quad;
using bst_fl256 = boost::multiprecision::cpp_bin_float_oct;
using bst_fl512 = bst_floatX<512, 23>;
using bst_fl1024 = bst_floatX<1024, 27>;
using bst_fl2048 = bst_floatX<2048, 31>;

using boost::multiprecision::uint128_t;
using boost::multiprecision::uint256_t;

void print_bin(const void *src, size_t bytes) noexcept;

void print_bin(uint128_t val, bool is_little_endian) noexcept {
  uint8_t buffer[4096];

  auto bytes = libHybractal::encode_uintX<uint128_t>(
                   val, buffer, sizeof(buffer), is_little_endian)
                   .value();
  print_bin(buffer, bytes);
}

void test_bin(bool quiet) noexcept;

void test_float128(const bst_fl128 &val) noexcept;

template <int precision>
void test_float_X(float_by_prec_t<precision> flt) noexcept {
  uint8_t buffer[precision * 4];
  constexpr size_t buffer_capacity = sizeof(buffer);

  auto encoded_bytes =
      libHybractal::encode_float(flt, buffer, buffer_capacity).value();

  assert(encoded_bytes == precision * sizeof(float));

  auto decoded_value = libHybractal::decode_float<float_by_prec_t<precision>>(
                           buffer, encoded_bytes)
                           .value();

  assert(decoded_value == flt);

  std::vector<uint8_t> binary;
  binary.resize(encoded_bytes);
  memcpy(binary.data(), buffer, encoded_bytes);

  auto encoded_bytes_2 =
      libHybractal::encode_float(decoded_value, buffer, sizeof(buffer)).value();

  assert(encoded_bytes_2 == binary.size());

  for (size_t idx = 0; idx < binary.size(); idx++) {
    assert(buffer[idx] == binary[idx]);
  }
}

int main() {
  constexpr size_t f512sz = sizeof(bst_fl512);
  test_bin(true);
  test_float128(bst_fl128(1) / 3);
  test_float128(bst_fl128(-1) / 3);
  test_float128(bst_fl128(-1) / bst_fl128("1e4933"));

  test_float_X<1>(114514);
  test_float_X<2>(114514);
  test_float_X<4>(114514);
  test_float_X<8>(114514);

  uint256_t u256;
  u256 = -1;

  std::vector<uint8_t> buffer;
  buffer.reserve(4096);

  boost::multiprecision::export_bits(u256, std::back_inserter(buffer), 8);

  // cout << "buffer.size() = " << buffer.size() << endl;

  cout << "Success" << endl;

  return 0;
}

void print_bin(const void *src, size_t bytes) noexcept {
  const uint8_t *const u8src = reinterpret_cast<const uint8_t *>(src);
  for (size_t i = 0; i < bytes; i++) {
    cout << fmt::format("{:08b} ", u8src[i]);
  }
  cout << endl;
}

void test_bin(bool quiet) noexcept {
  uint128_t u128 = 16;

  uint8_t buffer[4096];

  auto bytes =
      libHybractal::encode_uintX<uint128_t>(u128, buffer, sizeof(buffer), true)
          .value();
  if (!quiet) {
    cout << "Little endian: \n";
    print_bin(buffer, 128 / 8);
  }

  uint128_t another =
      libHybractal::decode_uintX<uint128_t>(buffer, bytes, true).value();
  assert(another == u128);

  bytes =
      libHybractal::encode_uintX<uint128_t>(u128, buffer, sizeof(buffer), false)
          .value();
  if (!quiet) {
    cout << "Big endian: \n";
    print_bin(buffer, 128 / 8);
  }

  another = libHybractal::decode_uintX<uint128_t>(buffer, bytes, false).value();
  assert(another == u128);

  uint64_t u64 = 0xFFF;

  libHybractal::encode_uintX<uint64_t>(u64, buffer, sizeof(buffer), true)
      .value();
  if (!quiet) {
    cout << "Little endian: \n";
    print_bin(buffer, 64 / 8);
  }
  assert(*reinterpret_cast<const uint64_t *>(buffer) == u64);

  libHybractal::encode_uintX<uint64_t>(u64, buffer, sizeof(buffer), false)
      .value();
  if (!quiet) {
    cout << "Big endian: \n";
    print_bin(buffer, 64 / 8);
  }
  assert(*reinterpret_cast<const uint64_t *>(buffer) != u64);

  // assert(0);
}

void test_float128(const bst_fl128 &val) noexcept {
  uint8_t buffer[4096];

  memset(buffer, 0xFF, sizeof(buffer));

  auto bytes =
      libHybractal::encode_boost_floatX(val, buffer, sizeof(buffer)).value();

  const bst_fl128 retake =
      libHybractal::decode_boost_floatX<bst_fl128>(buffer, bytes).value();

  assert(retake == val);
#ifdef __GNU__
  __float128 gcc_f128 = *reinterpret_cast<const __float128 *>(buffer);
#endif
  // assert(val == bst_fl128(gcc_f128));
}