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
#include <hex_convert.h>
#include <libHybractal.h>

#include <CLI11.hpp>
#include <float_encode.hpp>


std::string parse_input(std::string_view, int input_gen, int precision,
                        bool format_as_hex, std::string &err) noexcept;

int main(int argc, char **argv) {
  CLI::App app;
  std::string input_hex;
  app.add_option("input", input_hex)->required();

  std::string type;

  app.add_option("--format,-f", type, "Output format")
      ->default_val("hex")
      ->check(CLI::IsMember{{"hex", "float"}})
      ->expected(1);
  int precision;
  app.add_option("--precision,--prec,-p", precision, "Output precision")
      ->check(CLI::IsMember{{1, 2, 4, 8}})
      ->default_val(8)
      ->expected(1);

  int input_generation;
  app.add_option("--input-generation,--in-gen,--ig", input_generation,
                 "The generation of hex input")
      ->default_val(1)
      ->check(CLI::IsMember{{0, 1}});

  CLI11_PARSE(app, argc, argv);

  std::string err{};

  std::string result =
      parse_input(input_hex, input_generation, precision, (type == "hex"), err);

  if (!err.empty()) {
    std::cout << fmt::format("Invalid input: {}", err) << std::endl;
    return 1;
  }

  std::cout << result << std::endl;
  return 0;
}

std::string format_variant(const libHybractal::float_variant_t &var,
                           int precision, bool is_hex) noexcept;

std::string parse_input(std::string_view sv, int input_gen, int precision,
                        bool format_as_hex, std::string &err) noexcept {
  err.clear();

  if (sv.starts_with("0x") || sv.starts_with("0X")) {
    return parse_input({sv.begin() + 2, sv.end()}, input_gen, precision,
                       format_as_hex, err);
  }

  const size_t len = sv.length();

  if (len % 2 != 0) {
    err = "The lenght of string must be even number";
    return {};
  }

  std::string err1{}, err2{};
  auto var1 = libHybractal::hex_to_float_by_gen(
      {sv.data(), sv.data() + len / 2}, input_gen, err1);
  auto var2 = libHybractal::hex_to_float_by_gen(
      {sv.data() + len / 2, sv.data() + len}, input_gen, err2);

  if (!err1.empty() && !err2.empty()) {
    err = fmt::format("Center hex failed to parse. err1 = {}, err2 = {}", err1,
                      err2);
    return {};
  }

  std::string str1{}, str2{};

  str1 = format_variant(var1, precision, format_as_hex);
  str2 = format_variant(var2, precision, format_as_hex);

  if (!format_as_hex) {
    str2.append(" i");
  }

  return str1 + str2;
}

#define CENTERHEXCONVERT_MATCH_INDEX(index, var, val_for_format, ir) \
  case (index): {                                                    \
    auto val = std::get<(index)>(var);                               \
    val_for_format = double(val);                                    \
    ir = libHybractal::float_type_cvt<std::decay_t<decltype(val)>,   \
                                      float_by_prec_t<8>>(val);      \
  } break;

#define CHX_MATCH_PRECISION(ir, precision, hex, bytes)                     \
  case (precision): {                                                      \
    {                                                                      \
      float_by_prec_t<precision> val =                                     \
          libHybractal::float_type_cvt<decltype(ir),                       \
                                       float_by_prec_t<precision>>(ir);    \
      uint8_t buffer[4096];                                                \
      const size_t bin_bytes =                                             \
          libHybractal::encode_float(val, buffer, sizeof(buffer)).value(); \
      bytes = fractal_utils::bin_2_hex(buffer, bin_bytes, hex.data(),      \
                                       hex.size(), false);                 \
    }                                                                      \
    break;                                                                 \
  }

std::string format_variant(const libHybractal::float_variant_t &var,
                           int precision, bool is_hex) noexcept {
  double val_for_format = NAN;
  float_by_prec_t<8> ir;

  switch (var.index()) {
    CENTERHEXCONVERT_MATCH_INDEX(0, var, val_for_format, ir);
    CENTERHEXCONVERT_MATCH_INDEX(1, var, val_for_format, ir);
    CENTERHEXCONVERT_MATCH_INDEX(2, var, val_for_format, ir);
    CENTERHEXCONVERT_MATCH_INDEX(3, var, val_for_format, ir);
    default:
      abort();
  }

  if (is_hex) {
    std::string hex;
    hex.resize(4096);
    std::optional<size_t> bytes;

    switch (precision) {
      CHX_MATCH_PRECISION(ir, 1, hex, bytes);
      CHX_MATCH_PRECISION(ir, 2, hex, bytes);
      CHX_MATCH_PRECISION(ir, 4, hex, bytes);
      CHX_MATCH_PRECISION(ir, 8, hex, bytes);
      default:
        abort();
    }

    if (!bytes.has_value()) {
      abort();
    }

    hex.resize(bytes.value());
    return hex;
  }

  return fmt::format("{:+}", val_for_format);
}