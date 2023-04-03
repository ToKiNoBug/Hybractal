#ifndef HYBRACTAL_LIBHYRACTAL_FLOATENCODE_HPP
#define HYBRACTAL_LIBHYRACTAL_FLOATENCODE_HPP

#include <optional>

#include "libHybractal.h"
namespace libHybractal {
template <typename uintX_t>
constexpr int uintX_precision() {
  if constexpr (std::is_same_v<uintX_t, uint_by_prec_t<1>>) {
    return 1;
  }
  if constexpr (std::is_same_v<uintX_t, uint_by_prec_t<2>>) {
    return 2;
  }
  if constexpr (std::is_same_v<uintX_t, uint_by_prec_t<4>>) {
    return 4;
  }
  if constexpr (std::is_same_v<uintX_t, uint_by_prec_t<8>>) {
    return 8;
  }

  return -1;
}

template <typename flt_t>
constexpr int floatX_precision() {
  if constexpr (std::is_same_v<flt_t, float_by_prec_t<1>>) {
    return 1;
  }
  if constexpr (std::is_same_v<flt_t, float_by_prec_t<2>>) {
    return 2;
  }
  if constexpr (std::is_same_v<flt_t, float_by_prec_t<4>>) {
    return 4;
  }
  if constexpr (std::is_same_v<flt_t, float_by_prec_t<8>>) {
    return 8;
  }
  return -1;
}

template <typename uintX_t>
constexpr int uintX_bits() {
  constexpr int precision = uintX_precision<uintX_t>();
  static_assert(precision > 0);

  return precision * 32;
}

template <typename fltX_t>
constexpr int floatX_bits() {
  constexpr int precision = floatX_precision<fltX_t>();
  static_assert(precision > 0);

  return precision * 32;
}

namespace internal {

template <typename uintX_t>
void encode_uintX(const uintX_t &bin, void *void_dst,
                  bool is_little_endian = true) noexcept {
  constexpr int bits = uintX_bits<uintX_t>();
  static_assert(bits > 0);
  static_assert(bits % 8 == 0);
  uint8_t *u8_dst = reinterpret_cast<uint8_t *>(void_dst);
  constexpr int bytes = bits / 8;
  for (int byteid = 0; byteid < bytes; byteid++) {
    const int offset = byteid * 8;
    uint8_t val = uint8_t((bin >> offset) & uintX_t(0xFF));
    if (is_little_endian) {
      u8_dst[byteid] = val;
    } else {
      u8_dst[bytes - byteid - 1] = val;
    }
  }
}

template <typename uintX_t>
uintX_t decode_uintX(const void *void_src,
                     bool is_little_endian = true) noexcept {
  constexpr int bits = uintX_bits<uintX_t>();
  static_assert(bits > 0);
  static_assert(bits % 8 == 0);
  constexpr int bytes = bits / 8;

  const uint8_t *const u8_src = reinterpret_cast<const uint8_t *>(void_src);
  uintX_t ret{0};

  for (int byteid = 0; byteid < bytes; byteid++) {
    const int offset = byteid * 8;
    uintX_t val{~uintX_t(0)};
    if (is_little_endian) {
      val = u8_src[byteid];
    } else {
      val = u8_src[bytes - byteid - 1];
    }

    val = val << offset;
    ret |= val;
  }

  return ret;
}

template <typename flt_t>
void encode_boost_floatX(const flt_t &flt, void *dst) noexcept {
  static_assert(!std::is_trivial_v<flt_t>);

  constexpr int precision = floatX_precision<flt_t>();
  using uintX_t = uint_by_prec_t<precision>;

  constexpr int total_bits = uintX_bits<uintX_t>();

  constexpr int bits = flt_t::backend_type::bit_count;
  constexpr int bits_encoded = bits - 1;
  constexpr int exp_bits = total_bits - bits_encoded - 1;
  constexpr int64_t exp_bias = flt_t::backend_type::max_exponent;

  uintX_t bin{0};

  if (flt.sign() < 0) {
    bin |= 1;
  }
  bin = bin << exp_bits;

  bin |= (flt.backend().exponent() + exp_bias);
  bin = bin << bits_encoded;

  const uintX_t mask = (uintX_t(1) << bits_encoded) - 1;
  bin |= (uintX_t(flt.backend().bits()) & mask);

  encode_uintX<uintX_t>(bin, dst);
}

template <typename flt_t>
flt_t decode_boost_floatX(const void *src) noexcept {
  static_assert(!std::is_trivial_v<flt_t>);

  constexpr int precision = floatX_precision<flt_t>();
  using uintX_t = uint_by_prec_t<precision>;

  constexpr int total_bits = uintX_bits<uintX_t>();

  constexpr int bits = flt_t::backend_type::bit_count;
  constexpr int bits_encoded = bits - 1;
  constexpr int exp_bits = total_bits - bits_encoded - 1;
  constexpr int64_t exp_bias = flt_t::backend_type::max_exponent;

  const uintX_t bin = decode_uintX<uintX_t>(src);

  flt_t result;

  result.backend().sign() = bool(bin >> (total_bits - 1));

  const auto exp_value = (bin >> bits_encoded) & ((uintX_t(1) << exp_bits) - 1);

  using exp_t = std::decay_t<decltype(result.backend().exponent())>;

  result.backend().exponent() = exp_t(exp_value) - exp_bias;

  uintX_t mantissa = bin & ((uintX_t(1) << bits_encoded) - 1);

  if (exp_value != 0) {
    mantissa |= (uintX_t(1) << bits_encoded);
  }

  using bits_t = std::decay_t<decltype(result.backend().bits())>;

  // bits_t bits_val = mantissa;

  result.backend().bits() = mantissa.template convert_to<bits_t>();

  // result.backend().exponent()=

  return result;
}
}  // namespace internal

template <typename uintX_t>
std::optional<size_t> encode_uintX(const uintX_t &bin, void *dst,
                                   size_t capacity,
                                   bool is_little_endian) noexcept {
  constexpr int bits = uintX_bits<uintX_t>();
  if (capacity < size_t(bits / 8)) {
    return std::nullopt;
  }

  internal::encode_uintX<uintX_t>(bin, dst, is_little_endian);
  return size_t(bits / 8);
}

template <typename uintX_t>
std::optional<uintX_t> decode_uintX(const void *src, size_t src_bytes,
                                    bool is_little_endian) noexcept {
  constexpr int bits = uintX_bits<uintX_t>();
  constexpr size_t bytes = (bits / 8);

  if (bytes != src_bytes) {
    return std::nullopt;
  }

  return internal::decode_uintX<uintX_t>(src, is_little_endian);
}

template <typename flt_t>
std::optional<size_t> encode_boost_floatX(const flt_t &flt, void *dst,
                                          size_t capacity) noexcept {
  constexpr int precision = floatX_precision<flt_t>();
  static_assert(precision > 0);
  constexpr size_t bytes = precision * 4;

  if (capacity < bytes) {
    return std::nullopt;
  }

  internal::encode_boost_floatX(flt, dst);
  return bytes;
}

template <typename flt_t>
std::optional<flt_t> decode_boost_floatX(const void *src,
                                         size_t src_bytes) noexcept {
  constexpr int precision = floatX_precision<flt_t>();
  static_assert(precision > 0);

  constexpr size_t flt_bytes = precision * 4;

  if (src_bytes != flt_bytes) {
    return std::nullopt;
  }

  return internal::decode_boost_floatX<flt_t>(src);
}

}  // namespace libHybractal

#endif  // HYBRACTAL_LIBHYRACTAL_FLOATENCODE_HPP