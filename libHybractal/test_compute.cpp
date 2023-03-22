#include <chrono>
#include <cmath>
#include <iostream>
#include <libHybractal.h>
#include <random>

void test_single() noexcept;

int main(int, char **) {

  test_single();
  return 0;
}

void test_single() noexcept {
  constexpr bool ok = libHybractal::is_valid_string("010101");

  srand(std::time(nullptr));

  std::random_device rd;
  std::mt19937 mt(rand());

  std::uniform_real_distribution<double> rand(-2, 2);

  std::complex<double> z{0, 0}, C{rand(mt), rand(mt)};

  C *= 0.4;

  // libHybractal::sequence<0b10101, 5> seq;
  DECLARE_HYBRACTAL_SEQUENCE("10111011101") seq;

  const int age = seq.compute_age(z, C, INT16_MAX);

  std::cout << "age of " << C << " = " << age << std::endl;
}