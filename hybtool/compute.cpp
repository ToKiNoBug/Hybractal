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
#include <omp.h>

#include "hybtool.h"

bool run_compute(const task_compute &task) noexcept {
  omp_set_num_threads(task.threads);

  libHybractal::hybf_archive file(task.info.rows, task.info.cols,
                                  task.save_mat_z);

  file.metainfo() = task.info;
  fractal_utils::fractal_map mat_age = file.map_age();
  fractal_utils::fractal_map mat_z = file.map_z();

  double wtime;
  wtime = omp_get_wtime();
  libHybractal::compute_frame_by_precision(
      file.metainfo().window_base(), file.metainfo().precision(),
      file.metainfo().maxit, mat_age, file.have_mat_z() ? &mat_z : nullptr);
  wtime = omp_get_wtime() - wtime;

  if (task.bechmark) {
    std::cout << fmt::format("Computation cost {} seconds.\n", wtime);
  }

  wtime = omp_get_wtime();
  auto ret = file.save(task.filename);
  wtime = omp_get_wtime() - wtime;

  if (task.bechmark) {
    std::cout << fmt::format("Export cost {} seconds.\n", wtime);
  }

  return ret;
}