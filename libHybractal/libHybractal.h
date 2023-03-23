#ifndef HYBRACTAL_LIBHYBRACTAL_H
#define HYBRACTAL_LIBHYBRACTAL_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <complex>
#include <type_traits>
#include <vector>

namespace libHybractal {

static constexpr uint16_t maxit_max = UINT16_MAX - 1;

template <typename float_t>
inline std::complex<float_t> iterate_mandelbrot(
    std::complex<float_t> z, const std::complex<float_t> &C) noexcept {
  static_assert(std::is_arithmetic_v<float_t>);
  return z * z + C;
}

template <typename float_t>
inline std::complex<float_t> iterate_burningship(
    std::complex<float_t> z, const std::complex<float_t> &C) noexcept {
  static_assert(std::is_arithmetic_v<float_t>);

  z.real(std::abs(z.real()));
  z.imag(std::abs(z.imag()));

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

template <size_t N>
using const_str = char[N];

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

template <uint64_t bin, size_t len>
struct sequence {
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

#define DECLARE_HYBRACTAL_SEQUENCE(str)                                  \
  ::libHybractal::template sequence<::libHybractal::convert_to_bin(str), \
                                    ::libHybractal::static_strlen(str)>

}  // namespace libHybractal

#include <fractal_map.h>

namespace libHybractal {
void compute_frame(const fractal_utils::center_wind<double> &wind_C,
                   const uint16_t maxit,
                   fractal_utils::fractal_map &map_age_u16,
                   fractal_utils::fractal_map *map_z_nullable) noexcept;

}  // namespace libHybractal

#endif  // HYBRACTAL_LIBHYBRACTAL_H