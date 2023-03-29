#ifndef HYBRACTAL_LIBHYBRACTAL_H
#define HYBRACTAL_LIBHYBRACTAL_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <boost/multiprecision/complex_adaptor.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <complex>
#include <type_traits>
#include <vector>

template <size_t prec> struct float_prec_to_type {};

template <> struct float_prec_to_type<1> {
  using type = float;
};

template <> struct float_prec_to_type<2> {
  using type = double;
};

template <> struct float_prec_to_type<4> {
  using type = boost::multiprecision::cpp_bin_float_quad;
};

template <> struct float_prec_to_type<8> {
  using type = boost::multiprecision::cpp_bin_float_oct;
};

template <size_t prec>
using float_by_prec_t = typename float_prec_to_type<prec>::type;

// template <size_t prec>
// constexpr size_t float_encoded_bytes = prec * sizeof(float);

namespace libHybractal {

using hybf_float_t = float_by_prec_t<HYBRACTAL_FLT_PRECISION>;
using hybf_store_t = double;

constexpr size_t compute_t_size_v = sizeof(std::complex<hybf_float_t>);

template <typename src_t, typename dst_t = hybf_float_t>
inline dst_t float_type_cvt(const src_t &src) noexcept {
  constexpr bool is_src_trival = std::is_trivial_v<src_t>;
  constexpr bool is_dst_trival = std::is_trivial_v<dst_t>;

  if constexpr (std::is_same_v<src_t, dst_t>) {
    return src;
  }

  if constexpr (is_src_trival && is_dst_trival) {
    return dst_t(src);
  }

  if constexpr (!is_src_trival && is_dst_trival) {
    // convert from boost types to float/double
    return src.template convert_to<dst_t>();
  }

  if constexpr (is_src_trival && !is_dst_trival) {
    // convert from boost types to float/double
    return dst_t(src);
  }

  if constexpr (!is_src_trival && !is_dst_trival) {
    // convert from boost types to boost types
    return dst_t(src);
  }
  assert(false);
  return {};
}

constexpr size_t float_bytes(int precision) noexcept {
  switch (precision) {
  case 1:
    return sizeof(float_by_prec_t<1>);
  case 2:
    return sizeof(float_by_prec_t<2>);
  case 4:
    return sizeof(float_by_prec_t<4>);
  case 8:
    return sizeof(float_by_prec_t<8>);
  default:
    return SIZE_MAX;
  }
}
#if false
template <typename flt_t>
size_t encode_to_binary(const flt_t &flt, void *dst) noexcept {

  if constexpr (!std::is_trivial_v<flt_t>) {
    constexpr bool is_boost_quad =
        std::is_same_v<flt_t, boost::multiprecision::cpp_bin_float_quad>;
    constexpr bool is_boost_oct =
        std::is_same_v<flt_t, boost::multiprecision::cpp_bin_float_oct>;

    static_assert((is_boost_oct || is_boost_quad),
                  "No rule to encode floating point");

    if constexpr (is_boost_quad) {
      auto str = flt.str(256, std::ios::binary);
      memcpy(dst, str.c_str(), str.size());
      return str.size();
    }

    if constexpr (is_boost_oct) {
      auto str = flt.str(256, std::ios::binary);
      memcpy(dst, str.c_str(), str.size());
      return str.size();
    }
  }

  memcpy(dst, &flt, sizeof(flt));
  return sizeof(flt);
}

template <typename flt_t>
flt_t decode_from_binary(const void *src, size_t bytes,
                         std::string &err) noexcept {
  if constexpr (std::is_trivial_v<flt_t>) {
    assert(bytes == sizeof(flt_t));
    flt_t val;
    memcpy(&val, src, sizeof(val));
    return val;
  }

  if constexpr (!std::is_trivial_v<flt_t>) {
    constexpr bool is_boost_quad =
        std::is_same_v<flt_t, boost::multiprecision::cpp_bin_float_quad>;
    constexpr bool is_boost_oct =
        std::is_same_v<flt_t, boost::multiprecision::cpp_bin_float_oct>;

    static_assert((is_boost_oct || is_boost_quad),
                  "No rule to encode floating point");

    flt_t val(src);
    return val;
  }
}
#endif

hybf_float_t any_type_to_compute_t(const char *beg, const char *end,
                                   std::string &err,
                                   bool *is_same_type = nullptr) noexcept;

static constexpr uint16_t maxit_max = UINT16_MAX - 1;

template <typename float_t>
inline std::complex<float_t>
iterate_mandelbrot(std::complex<float_t> z,
                   const std::complex<float_t> &C) noexcept {
  return z * z + C;
}

template <typename flt_t> inline auto abs(flt_t val) {
  if (val >= 0) {
    return val;
  }
  return -val;
}

template <typename float_t>
inline std::complex<float_t>
iterate_burningship(std::complex<float_t> z,
                    const std::complex<float_t> &C) noexcept {
  z.real(abs(z.real()));
  z.imag(abs(z.imag()));

  return z * z + C;
}

template <typename float_t, bool is_mandelbrot>
inline std::complex<float_t> iterate(const std::complex<float_t> &z,
                                     const std::complex<float_t> &C) noexcept {
  if constexpr (is_mandelbrot) {
    return iterate_mandelbrot(z, C);
  } else {
    return iterate_burningship(z, C);
  }
}

template <typename float_t>
inline bool is_norm2_over_4(const std::complex<float_t> &z) noexcept {
  return (z.real() * z.real() + z.imag() * z.imag()) >= 4;
}

template <size_t N> using const_str = char[N];

template <size_t N>
constexpr bool is_valid_string(const const_str<N> &str) noexcept {
  for (size_t i = 0; i < N - 1; i++) {
    if (str[i] == '0' || str[i] == '1') {
      continue;
    }
    return false;
  }
  return true;
}

template <uint64_t bin, size_t len> struct sequence {
  static constexpr uint64_t binary = bin;
  static constexpr size_t length = len;

  static constexpr bool is_index_valid(size_t i) noexcept { return i < length; }

  static constexpr uint64_t mask_at(size_t i) noexcept {
    if (!is_index_valid(i)) {
      return 0ULL;
    }

    return (1ULL << (len - i - 1));
  }

  static constexpr bool value_at(size_t i) noexcept {
    return mask_at(i) & binary;
  }

  struct recurse_iterate_result {
    bool terminate_because_over_4{false};
    int it_times;
  };

  template <typename float_t, size_t idx>
  static recurse_iterate_result iterate_at(std::complex<float_t> &z,
                                           const std::complex<float_t> &C,
                                           int maxit) noexcept {
    static_assert(is_index_valid(idx));

    assert(!is_norm2_over_4(z));

    if (maxit <= 0) {
      return {false, 0};
    }

    const auto z_next = ::libHybractal::iterate<float_t, value_at(idx)>(z, C);

    const bool is_z_next_over_4 = is_norm2_over_4(z_next);
    // keep the latest value that not exceeds 4
    if (!is_z_next_over_4) {
      z = z_next;
    } else {
      // if it exceeds 4, stop
      return {is_z_next_over_4, 1};
    }

    // not exceeds 4
    if constexpr (idx == len - 1) {
      // terminate because reaches the end of a peroid
      return {is_z_next_over_4, 1};
    } else {
      // go on, and add the counter
      auto next = iterate_at<float_t, idx + 1>(z, C, maxit - 1);

      next.it_times++;
      return next;
    }
  }

  template <typename float_t>
  static recurse_iterate_result iterate(std::complex<float_t> &z,
                                        const std::complex<float_t> &C,
                                        int maxit) noexcept {
    return iterate_at<float_t, 0>(z, C, maxit);
  }

  template <typename float_t>
  static int compute_age(std::complex<float_t> &z,
                         const std::complex<float_t> &C,
                         const int maxit) noexcept {
    int counter = 0;

    if (is_norm2_over_4(z)) {
      return 0;
    }

    while (true) {
      if (counter >= maxit) {
        break;
      }
      recurse_iterate_result result = iterate<float_t>(z, C, maxit - counter);

      counter += result.it_times;

      if (result.terminate_because_over_4) {
        return counter;
      }
    }

    return -1;
  }
};

template <size_t N>
constexpr uint64_t convert_to_bin(const const_str<N> &str) noexcept {
  uint64_t bin = 0;
  for (size_t i = 0; str[i] != '\0'; i++) {
    bin = bin << 1;

    if (str[i] == '1') {
      bin = bin | 0b1;
    }
  }
  return bin;
}

template <size_t N>
constexpr uint64_t static_strlen(const const_str<N> &str) noexcept {
  return N - 1;
}

#define DECLARE_HYBRACTAL_SEQUENCE(str)                                        \
  ::libHybractal::template sequence<::libHybractal::convert_to_bin(str),       \
                                    ::libHybractal::static_strlen(str)>

constexpr uint64_t global_sequence_bin =
    ::libHybractal::convert_to_bin(HYBRACTAL_SEQUENCE_STR);
constexpr uint64_t global_sequence_len =
    ::libHybractal::static_strlen(HYBRACTAL_SEQUENCE_STR);
} // namespace libHybractal

#include <fractal_map.h>

namespace libHybractal {
void compute_frame(const fractal_utils::center_wind<hybf_float_t> &wind_C,
                   const uint16_t maxit,
                   fractal_utils::fractal_map &map_age_u16,
                   fractal_utils::fractal_map *map_z_nullable) noexcept;

} // namespace libHybractal

#endif // HYBRACTAL_LIBHYBRACTAL_H