#include <boost/multiprecision/cpp_bin_float.hpp>
#include <fmt/format.h>
#include <iostream>
#include <libHybractal.h>

using std::cout, std::endl;

int main() {
  using bst_fl128 = boost::multiprecision::cpp_bin_float_quad;

  bst_fl128 quad;

  constexpr size_t quad_bytes = sizeof(quad);

  constexpr size_t mantisa_count = decltype(quad)::backend_type::bit_count;

  quad = -0.3;

  std::vector<uint8_t> vec;

  auto ret = quad.convert_to<std::string>();

  cout << ret << endl;

  return 0;
}