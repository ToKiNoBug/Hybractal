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

#include "hybtool.h"
#include "libRender.h"
#include <fmt/format.h>
#include <iostream>
#include <omp.h>
#include <png_utils.h>

bool run_render(const task_render &task) noexcept {
  std::string err{""};
  std::vector<uint8_t> buffer;
  auto src = libHybractal::hybf_archive::load(task.hybf_file, buffer, &err);

  if (!err.empty()) {
    std::cout << "Failed to load source file, detail: " << err << std::endl;
    return false;
  }

  if (!src.have_mat_z()) {
    std::cout << "Source file doesn\'t contains mat-z.\n";
    std::cout << fmt::format("rows = {}, cols = {}, size of vector = {}",
                             src.rows(), src.cols(), src.mat_z_data().size())
              << std::endl;
    ;
    return false;
  }

  auto render_opt =
      libHybractal::hsv_render_option::load_from_file(task.json_file);

  if (!render_opt.has_value()) {
    std::cout << "Failed to load render option file" << std::endl;
    return false;
  }

  libHybractal::gpu_resource gpu_rcs(src.rows(), src.cols());

  if (!gpu_rcs.ok()) {
    std::cout << "Failed to initialize gpu resource." << std::endl;
    return false;
  }

  fractal_utils::fractal_map img_u8c3(src.rows(), src.cols(), 3);

  double wtime;
  wtime = omp_get_wtime();
  libHybractal::render_hsv(src.map_age(), src.map_z(), img_u8c3,
                           render_opt.value(), gpu_rcs);
  wtime = omp_get_wtime() - wtime;

  if (task.bechmark) {
    std::cout << fmt::format("Render cost {} seconds.\n", wtime);
  }

  wtime = omp_get_wtime();
  const bool ok = fractal_utils::write_png(
      task.png_file.c_str(), fractal_utils::color_space::u8c3, img_u8c3);
  wtime = omp_get_wtime() - wtime;

  if (!ok) {
    std::cout << "Failed to export png" << std::endl;
    return false;
  }

  if (task.bechmark) {
    std::cout << fmt::format("Image encoding cost {} seconds.\n", wtime);
  }

  return true;
}