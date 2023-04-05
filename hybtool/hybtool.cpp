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

#include <CLI11.hpp>
#include <thread>
#include <variant>

#include "hybtool.h"

std::array<libHybractal::hybf_float_t, 2> parse_center(
    const CLI::Option *opt_hex, std::string_view hex,
    const CLI::Option *opt_cf64, const std::array<double, 2> &f64,
    std::string &err) noexcept;

int main(int argc, char **argv) {
  CLI::App app;

  app.set_version_flag("--version,-v", HYBRACTAL_VERSION);

  bool show_config{false};

  app.add_flag("--show-config,--sc", show_config,
               "Show configuration when this program is built.")
      ->default_val(false);

  CLI::Validator is_hybf{[](std::string &input) -> std::string {
                           if (input.ends_with(".hybf")) {
                             return "";
                           }
                           return "Extension name must be .hybf";
                         },
                         "Extension name must be .hybf", "Is hybf"};

  //////////////////////////////////////
  CLI::App *const compute = app.add_subcommand(
      "compute", "Compute a fractal and generate a .hybf file.");

  task_compute task_c;
  compute->add_option("--rows,-r", task_c.info.rows, "Rows")
      ->required()
      ->check(CLI::PositiveNumber);
  compute->add_option("--cols,-c", task_c.info.cols, "Cols")
      ->required()
      ->check(CLI::PositiveNumber);
  compute->add_option("--maxit", task_c.info.maxit, "Max iteration")
      ->default_val(1024)
      ->check(CLI::Range(uint16_t(1), libHybractal::maxit_max));

  std::string center_hex;

  std::array<double, 2> center_f64;

  CLI::Option *const opt_center_double =
      compute->add_option("--center", center_f64, "Coordinate of center")
          ->expected(0, 1);
  CLI::Option *const opt_center_hex =
      compute
          ->add_option("--center-hex,--chx", center_hex,
                       "Coordiante of center, but encoded in byte sequence.")
          ->expected(0, 1);

  double x_span_f64{-1}, y_span_f64{-1};

  compute->add_option("--precision,-p", task_c.info.float_precision)
      ->check(CLI::IsMember{{1, 2, 4, 8}})
      ->required();
  compute
      ->add_option("--x-span,--span-x", x_span_f64,
                   "Range of x. Non-positive number means default value.")
      ->default_val(-1);
  compute->add_option("--y-span,--span-y", y_span_f64, "Range of y.")
      ->default_val(4);
  compute->add_option("-o", task_c.filename, "Generated hybf file.")
      ->default_val("out.hybf")
      ->check(is_hybf);
  compute
      ->add_option("--threads,-j", task_c.threads, "Threads used to compute.")
      ->default_val(std::thread::hardware_concurrency())
      ->check(CLI::PositiveNumber);
  compute->add_flag("--mat-z", task_c.save_mat_z, "Whether to save z matrix.")
      ->default_val(false);
  compute
      ->add_flag("--benchmark,--bench", task_c.bechmark,
                 "Show time costing for benchmark.")
      ->default_val(false);

  //////////////////////////////////////

  CLI::App *const render =
      app.add_subcommand("render", "Render a png image from a .hybf file.");
  task_render task_r;

  CLI::Validator is_json(
      [](std::string &str) -> std::string {
        if (str.ends_with(".json")) {
          return "";
        }
        return "Extension must be json.";
      },
      "Is json");

  render
      ->add_option("--json,--render-json,--rj", task_r.json_file,
                   "Renderer config json file.")
      ->check(CLI::ExistingFile & is_json)
      ->required();

  render
      ->add_option("source_file", task_r.hybf_file,
                   ".hybf file used to render.")
      ->check(CLI::ExistingFile & is_hybf)
      ->required();

  render->add_option("-o", task_r.png_file, "Generated png file.")
      ->default_val("out.png");
  render
      ->add_flag("--benchmark,--bench", task_r.bechmark,
                 "Show time costing for benchmark.")
      ->default_val(false);

  //////////////////////////////////////

  task_look task_l;
  CLI::App *const look = app.add_subcommand("look", "Browse hybf files.");
  look->add_option("file", task_l.file, "Files to look.")
      ->required()
      ->check(CLI::ExistingFile & is_hybf);

  look->add_flag("--all", task_l.show_all, "Show all metainfo.")
      ->default_val(false);
  look->add_flag("--blocks,--blk", task_l.show_blocks, "Show data blocks.")
      ->default_val(false);
  look->add_flag("--sequence,--seq", task_l.show_sequence,
                 "Show iteration sequence.")
      ->default_val(false);
  look->add_flag("--size", task_l.show_size, "Show rows and cols.")
      ->default_val(false);
  look->add_flag("--window,--wind", task_l.show_window, "Show compute window.")
      ->default_val(false);
  look->add_flag("--center-hex,--chx", task_l.show_center_hex,
                 "Show center hex.")
      ->default_val(false);
  look->add_flag("--maxit", task_l.show_maxit, "Show maxit.")
      ->default_val(false);
  look->add_flag("--float-precsion,--fpp", task_l.show_precision,
                 "Show floating point precsion when computation.")
      ->default_val(false);

  CLI::Validator is_zst{[](std::string &input) -> std::string {
                          if (input.ends_with(".zst")) {
                            return "";
                          }
                          return "Extension name must be .zst";
                        },
                        "Extension name must be .zst", "Is zst"};

  look->add_option("--extract-age-compressed,--eac",
                   task_l.extract_age_compressed,
                   "Extract compress age matrix.")
      ->check(is_zst & !CLI::ExistingFile);

  look->add_option("--extract-age-decompressed,--ea",
                   task_l.extract_age_decompress, "Extract age matrix.")
      ->check(!CLI::ExistingFile);

  look->add_option("--extract-z-compressed,--ezc", task_l.extract_z_compressed,
                   "Extract compressed z matrix.")
      ->check(is_zst & !CLI::ExistingFile);

  look->add_option("--extract-z-decompress,--ez", task_l.extract_z_decompress,
                   "Extract z matrix.")
      ->check(!CLI::ExistingFile);

  //////////////////////////////////////
  CLI11_PARSE(app, argc, argv);

  if (show_config) {
    std::cout << fmt::format(
                     "Configured with : HYBRACTAL_SEQUENCE_STR = {}, "
                     "floating point precision = {}, sizeof hybf_float_t = "
                     "{}.\nFloat128 backend is {}, float256 backend is {}.\n"
                     "CMAKE_BUILD_TYPE = {}",
                     HYBRACTAL_SEQUENCE_STR, HYBRACTAL_FLT_PRECISION,
                     sizeof(libHybractal::hybf_float_t),
                     HYBRACTAL_FLOAT128_BACKEND, HYBRACTAL_FLOAT256_BACKEND,
                     HYB_CMAKE_BUILD_TYPE)
              << std::endl;
  }

  if (compute->count() > 0) {
    task_c.info.wind.x_span = x_span_f64;
    task_c.info.wind.y_span = y_span_f64;

    task_c.override_x_span();
    std::string err;
    task_c.info.wind.center = parse_center(opt_center_hex, center_hex,
                                           opt_center_double, center_f64, err);

    if (!err.empty()) {
      std::cerr << "Failed to parse center. Details: " << err << std::endl;
      return 1;
    }

    if (!run_compute(task_c)) {
      std::cerr << "run_compute failed." << std::endl;
      return 1;
    }
  }

  if (render->count() > 0) {
    if (!run_render(task_r)) {
      std::cout << "Failed to render." << std::endl;
      return 1;
    }
  }

  if (look->count() > 0) {
    if (!run_look(task_l)) {
      std::cout << "Failed to lookup." << std::endl;
      return 1;
    }
  }

  std::cout << "Success" << std::endl;

  return 0;
}

std::array<libHybractal::hybf_float_t, 2> parse_center(
    const CLI::Option *opt_center_hex, std::string_view center_hex,
    const CLI::Option *opt_center_double,
    const std::array<double, 2> &center_f64, std::string &err) noexcept {
  err.clear();
  std::array<libHybractal::hybf_float_t, 2> result;

  if (opt_center_hex->count() > 0) {
    // if center hex is assigned, use center hex

    constexpr size_t bytes_center = sizeof(result);

    if (center_hex.size() != bytes_center &&
        center_hex.size() != (bytes_center + 2)) {
      err = fmt::format(
          "Invalid value for center_hex, expceted {} or {} characters",
          bytes_center, bytes_center + 2);
      return {};
    }

    auto ret =
        fractal_utils::hex_2_bin(center_hex, result.data(), sizeof(result));
    if (!ret.has_value() || ret.value() != sizeof(result)) {
      err = "Invalid value for center_hex";
      return {};
    }

    return result;
  }

  // center hex is not assigned, use center_f64

  if (opt_center_double->count() <= 0) {
    err = "No value for center";
    return {};
  }

  result[0] = center_f64[0];
  result[1] = center_f64[1];

  return result;
}