#ifndef HYBRACTAL_LIBHYRACTAL_FLOATENCODE_HPP
#define HYBRACTAL_LIBHYRACTAL_FLOATENCODE_HPP

#include <optional>

#include "libHybractal.h"

namespace libHybractal {

constexpr int guess_precision(size_t bytes_single_number, bool is_old) {
  const int bytes = bytes_single_number;
  if (bytes % 4 == 2) {
    return guess_precision(bytes - 2, is_old);
  }

  if (bytes == float_bytes(1)) {
    return 1;
  }
  if (bytes == float_bytes(2)) {
    return 2;
  }
  if ((bytes == float_bytes(4) && is_old)) {
    return 4;
  }
  if ((bytes == float_bytes(8)) && is_old) {
    return 8;
  }

  if ((bytes % 4 == 0) && !is_old) {
    return bytes / 4;
  }

  return -1;
}

template <typename flt_t>
concept is_boost_multiprecison_float = requires(const flt_t &flt) {
                                         requires !std::is_trivial_v<flt_t>;
                                         flt.backend();
                                         flt.backend().sign();
                                         flt.backend().exponent();
                                         flt.backend().bits();
                                         flt_t::backend_type::bit_count;
                                         flt_t::backend_type::max_exponent;
                                       };

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
  requires is_boost_multiprecison_float<flt_t>
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
  requires is_boost_multiprecison_float<flt_t>
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

  {
    using uint_bits_t =
        boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
            bits, bits, boost::multiprecision::unsigned_magnitude,
            boost::multiprecision::unchecked, void>>;

    uint_bits_t temp = mantissa.template convert_to<uint_bits_t>();

    result.backend().bits() = temp.backend();
  }

  return result;
}
} // namespace internal

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
  requires is_boost_multiprecison_float<flt_t>
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
  requires is_boost_multiprecison_float<flt_t>
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

template <typename flt_t>
std::optional<size_t> encode_float(const flt_t &flt, void *dst,
                                   size_t capacity) noexcept {
  constexpr bool is_trivial = std::is_trivial_v<flt_t>;
  constexpr bool is_boost = is_boost_multiprecison_float<flt_t>;

  constexpr bool is_known_type = is_boost || is_trivial;
  static_assert(is_known_type, "No way to serialize this type.");

  if constexpr (is_trivial) {
    constexpr size_t bytes = sizeof(flt_t);

    if (bytes > capacity) {
      return std::nullopt;
    }

    memcpy(dst, &flt, bytes);
    return bytes;
  }

  if constexpr (is_boost) {
    return encode_boost_floatX(flt, dst, capacity);
  }

  return std::nullopt;
}

template <typename flt_t>
std::optional<flt_t> decode_float(const void *src, size_t bytes) noexcept {
  constexpr bool is_trivial = std::is_trivial_v<flt_t>;
  constexpr bool is_boost = is_boost_multiprecison_float<flt_t>;

  constexpr bool is_known_type = is_boost || is_trivial;
  static_assert(is_known_type, "No way to serialize this type.");

  if constexpr (is_trivial) {
    if (bytes != sizeof(flt_t)) {
      return std::nullopt;
    }

    return *reinterpret_cast<const flt_t *>(src);
  }

  if constexpr (is_boost) {
    return decode_boost_floatX<flt_t>(src, bytes);
  }

  return std::nullopt;
}

template <typename flt_t>
std::optional<size_t> encode_array2(const std::array<flt_t, 2> &src, void *dst,
                                    size_t capacity) noexcept {
  try {
    auto first_bytes = encode_float(src[0], dst, capacity).value();
    auto next_bytes = encode_float(src[1], dst, capacity - first_bytes).value();
    return first_bytes + next_bytes;
  } catch (...) {
    return std::nullopt;
  }
}

template <typename flt_t>
std::optional<size_t> encode_complex(const std::complex<flt_t> &src, void *dst,
                                     size_t capacity) noexcept {
  return encode_array2({src.real(), src.imag()}, dst, capacity);
}

template <typename flt_t>
std::optional<std::array<flt_t, 2>> decode_array2(const void *src,
                                                  size_t bytes) noexcept {
  if (bytes % 2 != 0) {
    return std::nullopt;
  }

  const size_t offset = bytes / 2;
  std::array<flt_t, 2> ret;
  try {
    ret[0] = decode_float<flt_t>(src, offset).value();
    ret[1] =
        decode_float<flt_t>(((const uint8_t *)src + offset), offset).value();
  } catch (...) {
    return std::nullopt;
  }

  return ret;
}

template <typename flt_t>
std::optional<std::complex<flt_t>> decode_complex(const void *src,
                                                  size_t bytes) noexcept {
  auto array2 = decode_array2<flt_t>(src, bytes);

  if (array2.has_value()) {
    auto temp = array2.value();
    return std::complex<flt_t>{temp[0], temp[1]};
  }
  return std::nullopt;
}

} // namespace libHybractal

#endif // HYBRACTAL_LIBHYRACTAL_FLOATENCODE_HPP