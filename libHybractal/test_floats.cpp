#include <fmt/format.h>
#include <libHybractal.h>

#include <boost/multiprecision/cpp_bin_float.hpp>
#include <float_encode.hpp>
#include <iostream>

using std::cout, std::endl;

using bst_fl128 = boost::multiprecision::cpp_bin_float_quad;
using bst_fl256 = boost::multiprecision::cpp_bin_float_oct;
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

void encode_float128(const bst_fl128 &val) noexcept;

void encode_float256(const bst_fl256 &val) noexcept;

void test_float128(const bst_fl128 &val) noexcept;

int main() {
  test_bin(true);
  test_float128(bst_fl128(1) / 3);
  test_float128(bst_fl128(-1) / 3);
  test_float128(bst_fl128(-1) / bst_fl128("1e4933"));

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
}

void encode_float128(const bst_fl128 &flt) noexcept {
  uint128_t bin{0};

  constexpr int bits = bst_fl128::backend_type::bit_count;
  constexpr int bits_encoded = bits - 1;
  constexpr int exp_bits = 128 - (bits_encoded)-1;
  constexpr int exp_offset = bst_fl128::backend_type::max_exponent;

  if (flt.sign() < 0) {
    bin |= 1;
  }
  bin = bin << exp_bits;

  bin |= (flt.backend().exponent() + exp_offset);
  bin = bin << bits_encoded;

  const uint128_t mask = (uint128_t(1) << bits_encoded) - 1;

  bin |= (uint128_t(flt.backend().bits()) & mask);

  print_bin(bin, true);
}

void encode_float256(const bst_fl256 &val) noexcept {
  uint8_t buffer[4096];

  libHybractal::encode_boost_floatX<bst_fl256>(val, buffer, sizeof(buffer));

  print_bin(buffer, 256 / 8);
}

void decode_float128(const uint128_t &bin) noexcept {
  bst_fl128 flt;

  flt.backend().sign() = bool(bin >> 127);
}

void test_float128(const bst_fl128 &val) noexcept {
  uint8_t buffer[4096];

  memset(buffer, 0xFF, sizeof(buffer));

  auto bytes =
      libHybractal::encode_boost_floatX(val, buffer, sizeof(buffer)).value();

  const bst_fl128 retake =
      libHybractal::decode_boost_floatX<bst_fl128>(buffer, bytes).value();

  assert(retake == val);

  __float128 gcc_f128 = *reinterpret_cast<const __float128 *>(buffer);

  // assert(val == bst_fl128(gcc_f128));
}