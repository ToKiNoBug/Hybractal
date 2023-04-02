#include <fmt/format.h>
#include <libHybractal.h>

#include <boost/multiprecision/cpp_bin_float.hpp>
#include <iostream>
#include <type_traits>

using std::cout, std::endl;

void print_f128(const boost::multiprecision::cpp_bin_float_quad& f) noexcept;

std::string u128_to_bin(const boost::multiprecision::uint128_t& bin) noexcept;

int main() {
  using bst_fl128 = boost::multiprecision::cpp_bin_float_quad;

  bst_fl128 quad;

  constexpr size_t quad_bytes = sizeof(quad);

  constexpr size_t mantisa_count = decltype(quad)::backend_type::bit_count;

  quad = 0.3;

  print_f128(-quad);

  std::vector<uint8_t> vec;

  auto ret = quad.convert_to<std::string>();

  cout << ret << endl;

  using uint128_t = boost::multiprecision::uint128_t;

  uint128_t bin{0};

  constexpr int exp_bits = libHybractal::internal::highest_positive_bit(
      bst_fl128::backend_type::max_exponent);

  cout << fmt::format("bst_fl128::backend_type::max_exponent = {:0b}\n",
                      bst_fl128::backend_type::max_exponent);

  bin |= (quad.sign() > 0 ? 0 : 1);
  bin = bin << exp_bits;
  {
    uint128_t val =
        quad.backend().exponent() + bst_fl128::backend_type::max_exponent;
    assert(val >= 0);
    // bin |= val;
    bin = bin << std::decay_t<decltype(quad.backend())>::bit_count;
  }
  constexpr int bit_count = std::decay_t<decltype(quad.backend())>::bit_count;

  static_assert(bit_count + exp_bits + 1 == 128);

  using bits_t = decltype(quad.backend().bits());

  constexpr bool is_same = std::is_same_v<uint128_t, std::decay_t<bits_t>>;
  {
    uint128_t val = quad.backend().bits();
    uint128_t mask = (uint128_t(1) << (bit_count)) - 1;

    assert(val >= 0);
    bin |= (val & mask);
  }
  std::string bits = u128_to_bin(bin);
  cout << bits << endl;

  return 0;
}

void print_f128(const boost::multiprecision::cpp_bin_float_quad& f) noexcept {
  cout << fmt::format("sign bit = {}\n", f.sign());
  uint16_t val{0};
  val |= f.backend().exponent() +
         std::decay_t<decltype(f.backend())>::max_exponent;
  cout << fmt::format("exponent = {:015b}\n", val);

  cout << fmt::format("bits = \n{}\n", u128_to_bin(f.backend().bits()));
  //__int128 i128;
  // cout << "bits = " << __int128(f.backend().bits().limbs()) << '\n';
}

std::string u128_to_bin(const boost::multiprecision::uint128_t& bin) noexcept {
  std::string bits;

  for (int bid = 15; bid >= 0; bid--) {
    uint8_t value = ((bin >> (bid * 8)) & 0xFF).convert_to<uint8_t>();

    bits += fmt::format("{:08b} ", int(value));
  }
  return bits;
}