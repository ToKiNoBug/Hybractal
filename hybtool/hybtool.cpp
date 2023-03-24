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

#include <hex_convert.h>

#include <CLI11.hpp>
#include <thread>
#include <variant>

#include "hybtool.h"
#include <fmt/format.h>

int main(int argc, char **argv) {
  CLI::App app;

  bool show_config{false};

  app.add_flag("--show-config,--sc", show_config,
               "Show configuration when this program is built.")
      ->default_val(false);

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

  CLI::Option *const opt_center_double =
      compute
          ->add_option("--center", task_c.info.window_center,
                       "Coordinate of center")
          ->expected(0, 1);
  CLI::Option *const opt_center_hex =
      compute
          ->add_option("--center-hex,--chx", center_hex,
                       "Coordiante of center, but encoded in byte sequence.")
          ->expected(0, 1);

  compute
      ->add_option("--x-span,--span-x", task_c.info.window_xy_span[0],
                   "Range of x. Non-positive number means default value.")
      ->default_val(-1);
  compute
      ->add_option("--y-span,--span-y", task_c.info.window_xy_span[1],
                   "Range of y.")
      ->default_val(2);
  compute->add_option("-o", task_c.filename, "Generated hybf file.")
      ->default_val("out.hybf");
  compute
      ->add_option("--threads,-j", task_c.threads, "Threads used to compute.")
      ->default_val(std::thread::hardware_concurrency())
      ->check(CLI::PositiveNumber);
  compute->add_flag("--mat-z", task_c.save_mat_z, "Whether to save z matrix.")
      ->default_val(false);

  //////////////////////////////////////

  CLI::App *const render =
      app.add_subcommand("render", "Render a png image from a .hybf file.");
  task_render task_r;

  render
      ->add_option("--json,--render-json,--rj", task_r.json_file,
                   "Renderer config json file.")
      ->check(CLI::ExistingFile)
      ->required();

  render
      ->add_option("source_file", task_r.hybf_file,
                   ".hybf file used to render.")
      ->check(CLI::ExistingFile)
      ->required();

  render->add_option("-o", task_r.png_file, "Generated png file.")
      ->default_val("out.png");

  //////////////////////////////////////

  task_look task_l;
  CLI::App *const look = app.add_subcommand("look", "Browse hybf files.");
  look->add_option("file", task_l.file, "Files to look.")
      ->required()
      ->check(CLI::ExistingFile);
  look->add_flag("--blocks,--blk", task_l.show_blocks)->default_val(false);
  look->add_flag("--sequence,--seq", task_l.show_sequence)->default_val(false);
  look->add_flag("--size", task_l.show_size, "Show rows and cols.")
      ->default_val(false);
  look->add_flag("--window,--wind", task_l.show_window)->default_val(false);
  look->add_flag("--center-hex,--chx", task_l.show_center_hex)
      ->default_val(false);
  look->add_flag("--maxit", task_l.show_maxit)->default_val(false);

  CLI::Validator zst{[](std::string &input) -> std::string {
                       if (input.ends_with(".zst")) {
                         return "";
                       }
                       return "Extension name must be .zst";
                     },
                     "Extension name must be .zst", "Extension name check"};

  look->add_option("--extract-age-compressed,--eac",
                   task_l.extract_age_compressed)
      ->check(zst & !CLI::ExistingFile);

  look->add_option("--extract-age-decompressed,--ead",
                   task_l.extract_age_decompress)
      ->check(!CLI::ExistingFile);

  look->add_option("--extract-z-compressed,--ezc", task_l.extract_z_compressed)
      ->check(zst & !CLI::ExistingFile);

  look->add_option("--extract-z-decompress,--ezd", task_l.extract_z_decompress)
      ->check(!CLI::ExistingFile);

  //////////////////////////////////////
  CLI11_PARSE(app, argc, argv);

  if (show_config) {
    std::cout << fmt::format("Configured with : HYBRACTAL_SEQUENCE_STR = {}",
                             HYBRACTAL_SEQUENCE_STR)
              << std::endl;
  }

  if (compute->count() > 0) {
    task_c.override_x_span();

    if (opt_center_hex->count() > 0) {
      if (center_hex.size() != 32 && center_hex.size() != 34) {
        std::cerr
            << "Invalid value for center_hex, expceted 32 or 34 characters"
            << std::endl;
        return 1;
      }

      auto ret =
          fractal_utils::hex_2_bin(center_hex, task_c.info.window_center.data(),
                                   sizeof(task_c.info.window_center));
      if (!ret.has_value() ||
          ret.value() != sizeof(task_c.info.window_center)) {
        std::cerr << "Invalid value for center_hex" << std::endl;
        return 1;
      }
    } else {
      if (opt_center_double->count() <= 0) {
        std::cerr << "No value for center" << std::endl;
        return 1;
      }
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
