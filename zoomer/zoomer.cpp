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
#include <fractal_colors.h>
#include <libHybfile.h>
#include <libRender.h>
#include <omp.h>
#include <render_utils.h>
#include <zoom_utils.h>

#include <CLI11.hpp>
#include <QApplication>
#include <QMainWindow>
#include <iostream>
#include <memory>

void compute_fun(const fractal_utils::wind_base &, void *custom_ptr,
                 fractal_utils::fractal_map *map_fractal);

void render_fun(const fractal_utils::fractal_map &map_fractal,
                const fractal_utils::wind_base &window, void *custom_ptr,
                fractal_utils::fractal_map *map_u8c3_do_not_resize);

bool export_fun(const fractal_utils::fractal_map &map_fractal,
                const fractal_utils::wind_base &window, void *custom_ptr,
                const fractal_utils::fractal_map &map_u8c3_do_not_resize,
                const char *filename);

struct metainfo4gui_s {
  libHybractal::hybf_metainfo_new info;
  fractal_utils::fractal_map mat_z{0, 0, 16};

  libHybractal::gpu_resource gpu_rcs;
  libHybractal::hsv_render_option renderer;
};

metainfo4gui_s get_info_struct(std::string_view filename,
                               std::string_view json) noexcept;

#define ZOOMER_PRIVATE_MATCH_PRECISION(precision)                \
  case (precision):                                              \
    window_ptr.reset(new fractal_utils::mainwindow(              \
        float_by_prec_t<(precision)>(0.0), nullptr, window_size, \
        sizeof(uint16_t)));                                      \
    break;

int main(int argc, char **argv) {
  omp_set_num_threads(20);

  CLI::App capp;

  std::string source_file{""};
  capp.add_option("hybf_file", source_file, "Source file for metainfo.")
      ->check(CLI::ExistingFile)
      ->default_val("default.hybf");
  std::string render_json{""};
  capp.add_option("--render-json,--rj", render_json, "Render config file.")
      ->check(CLI::ExistingFile)
      ->default_val("render1.json");

  int maxit_override{-1};

  capp.add_option("--override-maxit,--omaxit", maxit_override,
                  "Override the maxit assigned by hybf file. Negative number "
                  "means no override.")
      ->default_val(-1);

  CLI11_PARSE(capp, argc, argv);

  metainfo4gui_s metainfo = get_info_struct(source_file, render_json);

  if (maxit_override > 0) {
    metainfo.info.maxit = maxit_override;
  }

  QApplication qapp(argc, argv);

  const std::array<int, 2> window_size{(int)metainfo.info.rows,
                                       (int)metainfo.info.cols};
  std::unique_ptr<fractal_utils::mainwindow> window_ptr{nullptr};

  switch (metainfo.info.precision()) {
    ZOOMER_PRIVATE_MATCH_PRECISION(1);
    ZOOMER_PRIVATE_MATCH_PRECISION(2);
    ZOOMER_PRIVATE_MATCH_PRECISION(4);
    ZOOMER_PRIVATE_MATCH_PRECISION(8);
    default:
      abort();
  }

  window_ptr->set_window(metainfo.info.window_base());
  window_ptr->display_range();

  window_ptr->callback_compute_fun = compute_fun;
  window_ptr->callback_render_fun = render_fun;
  window_ptr->callback_export_fun = export_fun;

  window_ptr->frame_file_extension_list = "*.hybf";

  window_ptr->custom_parameters = &metainfo;

  window_ptr->show();
  window_ptr->compute_and_paint();

  return qapp.exec();
}

metainfo4gui_s get_info_struct(std::string_view filename,
                               std::string_view json) noexcept {
  std::string err;
  auto archive = libHybractal::hybf_archive::load(filename, &err);

  if (!err.empty()) {
    std::cerr << "Failed to load source file, detail: " << err << std::endl;
    exit(1);
  }

  auto renderer = libHybractal::hsv_render_option::load_from_file(json);
  if (!renderer.has_value()) {
    std::cerr << "Failed to parse renderer." << std::endl;
    exit(1);
  }

  return metainfo4gui_s{
      archive.metainfo(),
      fractal_utils::fractal_map{
          archive.rows(), archive.cols(),
          sizeof(std::complex<libHybractal::hybf_store_t>)},
      libHybractal::gpu_resource{archive.rows(), archive.cols()},
      renderer.value()};
}

void compute_fun(const fractal_utils::wind_base &__wind, void *custom_ptr,
                 fractal_utils::fractal_map *map_fractal) {
  if (false) {
    std::cout << fmt::format(
                     "wind : center = [{}, {}], x_span = {}, y_span = {}",
                     __wind.displayed_center()[0], __wind.displayed_center()[1],
                     __wind.displayed_x_span(), __wind.displayed_y_span())
              << std::endl;
  }

  auto *metainfo = reinterpret_cast<metainfo4gui_s *>(custom_ptr);
  if (false) std::cout << "maxit = " << metainfo->info.maxit << std::endl;

  libHybractal::compute_frame_by_precision(__wind, metainfo->info.precision(),
                                           metainfo->info.maxit, *map_fractal,
                                           &metainfo->mat_z);
}

void render_fun(const fractal_utils::fractal_map &map_fractal,
                const fractal_utils::wind_base &, void *custom_ptr,
                fractal_utils::fractal_map *map_u8c3) {
  auto *metainfo = reinterpret_cast<metainfo4gui_s *>(custom_ptr);

  if (!metainfo->gpu_rcs.ok()) {
    std::cout << "Failed to acqurire gpu resource. exit." << std::endl;
    exit(1);
  }

  libHybractal::render_hsv(map_fractal, metainfo->mat_z, *map_u8c3,
                           metainfo->renderer, metainfo->gpu_rcs);
}

bool export_fun(const fractal_utils::fractal_map &map_fractal,
                const fractal_utils::wind_base &__wind, void *custom_ptr,
                const fractal_utils::fractal_map &map_u8c3_do_not_resize,
                const char *filename) {
  auto *metainfo = reinterpret_cast<metainfo4gui_s *>(custom_ptr);

  libHybractal::hybf_archive archive{metainfo->info.rows, metainfo->info.cols,
                                     true};

  {
    archive.metainfo().maxit = metainfo->info.maxit;
    __wind.copy_to(libHybractal::extract_wind_base(archive.metainfo().wind));
  }

  memcpy(archive.map_age().data, map_fractal.data, map_fractal.byte_count());
  memcpy(archive.map_z().data, metainfo->mat_z.data,
         metainfo->mat_z.byte_count());

  return archive.save(filename);
}