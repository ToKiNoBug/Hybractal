
#include <boost/multiprecision/cpp_bin_float.hpp>

int main() {
  boost::multiprecision::cpp_bin_float_quad quad;

  quad = 4;

  double val;
  val = quad.convert_to<double>();
}