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
  libHybractal::hybf_metainfo info;
  fractal_utils::fractal_map mat_z{0, 0, 16};

  libHybractal::gpu_resource gpu_rcs;
  libHybractal::hsv_render_option renderer;
};

metainfo4gui_s get_info_struct(std::string_view filename,
                               std::string_view json) noexcept;

int main(int argc, char **argv) {
  omp_set_num_threads(20);

  CLI::App capp;

  std::string source_file{""};
  capp.add_option("hybf_file", source_file)
      ->check(CLI::ExistingFile)
      ->default_val("default.hybf");
  std::string render_json{""};
  capp.add_option("--render-json,--rj", render_json)
      ->check(CLI::ExistingFile)
      ->default_val("render1.json");

  CLI11_PARSE(capp, argc, argv);

  metainfo4gui_s metainfo = get_info_struct(source_file, render_json);

  QApplication qapp(argc, argv);

  const std::array<int, 2> window_size{(int)metainfo.info.rows,
                                       (int)metainfo.info.cols};

  fractal_utils::mainwindow window(0.0, nullptr, window_size, 2);

  window.set_window(metainfo.info.window());
  window.display_range();

  window.callback_compute_fun = compute_fun;
  window.callback_render_fun = render_fun;
  window.callback_export_fun = export_fun;

  window.frame_file_extension_list = "*.hybf";

  window.custom_parameters = &metainfo;

  window.show();
  window.compute_and_paint();

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
      fractal_utils::fractal_map{archive.rows(), archive.cols(),
                                 sizeof(std::complex<double>)},
      libHybractal::gpu_resource{archive.rows(), archive.cols()},
      renderer.value()};
}

void compute_fun(const fractal_utils::wind_base &__wind, void *custom_ptr,
                 fractal_utils::fractal_map *map_fractal) {
  const auto wind =
      dynamic_cast<const fractal_utils::center_wind<double> &>(__wind);
  if (false) {
    std::cout << fmt::format(
                     "wind : center = [{}, {}], x_span = {}, y_span = {}",
                     wind.center[0], wind.center[1], wind.x_span, wind.y_span)
              << std::endl;
  }

  auto *metainfo = reinterpret_cast<metainfo4gui_s *>(custom_ptr);
  if (false)
    std::cout << "maxit = " << metainfo->info.maxit << std::endl;

  libHybractal::compute_frame(wind, metainfo->info.maxit, *map_fractal,
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

  const auto wind =
      dynamic_cast<const fractal_utils::center_wind<double> &>(__wind);

  {
    archive.metainfo().maxit = metainfo->info.maxit;
    archive.metainfo().window_center = wind.center;
    archive.metainfo().window_xy_span = {wind.x_span, wind.y_span};
  }

  memcpy(archive.map_age().data, map_fractal.data, map_fractal.byte_count());
  memcpy(archive.map_z().data, metainfo->mat_z.data,
         metainfo->mat_z.byte_count());

  return archive.save(filename);
}