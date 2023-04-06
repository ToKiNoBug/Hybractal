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
#include <libRender.h>
#include <omp.h>
#include <png_utils.h>

#include <atomic>
#include <filesystem>
#include <iostream>
#include <mutex>

#include "videotool.h"


using std::cout, std::cerr, std::endl;

std::vector<int> pngs_missing(const common_info &ci,
                              const render_task &rt) noexcept;

bool run_render(const common_info &ci, const render_task &rt) noexcept {
  libHybractal::hsv_render_option render;
  {
    auto temp = libHybractal::hsv_render_option::load_from_file(rt.config_file);
    if (!temp.has_value()) {
      cerr << fmt::format("Failed to load render json file {}.", rt.config_file)
           << endl;
      return false;
    }

    render = temp.value();
  }

  // read archives and check
  std::vector<libHybractal::hybf_archive> archives;
  archives.reserve(ci.frame_num);

  {
    std::vector<uint8_t> buffer;
    for (int fidx = 0; fidx < ci.frame_num; fidx++) {
      bool exists;
      std::string filename = hybf_filename(ci, fidx);
      libHybractal::hybf_archive temp;
      check_hybf_option opt;
      opt.move_archive = &temp;
      opt.nocheck_sequence = true;

      if (!check_hybf(filename, ci, buffer, exists, opt)) {
        if (exists) {
          cerr << fmt::format("Source file {} exists, but it is invalid.",
                              filename)
               << endl;
        } else {
          cerr << fmt::format("Source file {} is missing.", filename) << endl;
        }
        return false;
      }

      archives.emplace_back(std::move(temp));
    }
  }

  const auto frames_to_render = pngs_missing(ci, rt);

  omp_set_num_threads(rt.threads);
  std::atomic<int> error_counter{0};
  std::atomic<int> rendered_frame_counter =
      ci.frame_num - frames_to_render.size();
  std::mutex cout_lock;
#pragma omp parallel for schedule(dynamic)
  for (int taskidx = 0; taskidx < frames_to_render.size(); taskidx++) {
    const int fidx = frames_to_render[taskidx];
    if (cout_lock.try_lock()) {
      cout << fmt::format("[{:^6.1f}% : {:^3} / {:^3}] : rendering {}",
                          100 * float(rendered_frame_counter) /
                              std::max(ci.frame_num, 1),
                          rendered_frame_counter, ci.frame_num,
                          hybf_filename(ci, fidx))
           << endl;
      cout_lock.unlock();
    }
    thread_local libHybractal::gpu_resource gpu_rcs(ci.rows, ci.cols);
    if (!gpu_rcs.ok()) {
      cerr << "Fatal error : failed to initialize gpu resource." << endl;
      exit(1);
    }
    thread_local fractal_utils::fractal_map mat_u8c3(ci.rows, ci.cols, 3);

    auto &archive = archives[fidx];
    libHybractal::render_hsv(archive.map_age(), archive.map_z(), mat_u8c3,
                             render, gpu_rcs);
    std::vector<const void *> row_ptrs;

    for (int pngidx = 0; pngidx < rt.png_per_frame + rt.extra_png_num;
         pngidx++) {
      std::string pfilename = png_filename(ci, fidx, pngidx);

      const int skip_rows =
          fractal_utils::skip_rows(ci.rows, ci.ratio, rt.png_per_frame, pngidx);
      const int skip_cols =
          fractal_utils::skip_cols(ci.cols, ci.ratio, rt.png_per_frame, pngidx);
      const int image_rows = ci.rows - 2 * skip_rows;
      const int image_cols = ci.cols - 2 * skip_cols;
      row_ptrs.resize(image_rows);
      for (auto &ptr : row_ptrs) {
        ptr = nullptr;
      }

      for (int idx = 0; idx < image_rows; idx++) {
        const int r = idx + skip_rows;
        row_ptrs[idx] =
            mat_u8c3.address<fractal_utils::pixel_RGB>(r, skip_cols);
      }

      const bool ok = fractal_utils::write_png(
          pfilename.c_str(), fractal_utils::color_space::u8c3, row_ptrs.data(),
          image_rows, image_cols);
      if (!ok) {
        cerr << fmt::format("Failed to export {} for frame {}", pfilename, fidx)
             << endl;
        error_counter++;
        continue;
      }
    }
    rendered_frame_counter++;
  }

  cout << fmt::format(
              "[{:^6.1f}% : {:^3} / {:^3}] : All tasks finished({} "
              "finished with error).",
              100.0f, rendered_frame_counter, ci.frame_num, error_counter)
       << endl;

  return error_counter == 0;
}

std::vector<int> pngs_missing(const common_info &ci,
                              const render_task &rt) noexcept {
  std::vector<int> ret;
  ret.reserve(ci.frame_num);

  for (int fidx = 0; fidx < ci.frame_num; fidx++) {
    bool have_missing{false};
    for (int pidx = 0; pidx < rt.png_per_frame + rt.extra_png_num; pidx++) {
      std::string filename = png_filename(ci, fidx, pidx);
      if (!std::filesystem::is_regular_file(filename)) {
        have_missing = true;
        break;
      }
    }

    if (have_missing) {
      ret.emplace_back(fidx);
    }
  }

  return ret;
}