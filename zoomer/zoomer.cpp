#include <CLI11.hpp>
#include <QApplication>
#include <QMainWindow>
#include <fmt/format.h>
#include <fractal_colors.h>
#include <iostream>
#include <libHybfile.h>
#include <omp.h>
#include <render_utils.h>
#include <zoom_utils.h>

void compute_fun(const fractal_utils::wind_base &, void *custom_ptr,
                 fractal_utils::fractal_map *map_fractal);

void render_fun(const fractal_utils::fractal_map &map_fractal,
                const fractal_utils::wind_base &window, void *custom_ptr,
                fractal_utils::fractal_map *map_u8c3_do_not_resize);

struct metainfo4gui_s {
  libHybractal::hybf_metainfo info;
  const bool have_mat_z;
  fractal_utils::fractal_map mat_z{0, 0, 16};

  fractal_utils::fractal_map mat_f32;
  float num_peroid{4};
};

metainfo4gui_s get_info_struct(std::string_view filename) noexcept;

int main(int argc, char **argv) {

  omp_set_num_threads(20);

  CLI::App capp;

  std::string source_file{""};
  capp.add_option("hybf_file", source_file)
      ->check(CLI::ExistingFile)
      ->required();

  CLI11_PARSE(capp, argc, argv);

  metainfo4gui_s metainfo = get_info_struct(source_file);

  QApplication qapp(argc, argv);

  const std::array<int, 2> window_size{(int)metainfo.info.rows,
                                       (int)metainfo.info.cols};

  fractal_utils::mainwindow window(0.0, nullptr, window_size, 2);

  window.set_window(metainfo.info.window());
  window.display_range();

  window.callback_compute_fun = compute_fun;
  window.callback_render_fun = render_fun;

  window.custom_parameters = &metainfo;

  window.show();
  window.compute_and_paint();

  return qapp.exec();
}

metainfo4gui_s get_info_struct(std::string_view filename) noexcept {

  std::string err;
  auto archive = libHybractal::hybf_archive::load(filename, &err);

  if (!err.empty()) {
    std::cerr << "Failed to load source file, detail: " << err << std::endl;
    exit(1);
  }

  fractal_utils::fractal_map map_f32{archive.rows(), archive.cols(),
                                     sizeof(float)};

  if (archive.have_mat_z()) {
    return metainfo4gui_s{
        archive.metainfo(), true,
        fractal_utils::fractal_map{archive.rows(), archive.cols(),
                                   sizeof(std::complex<double>)},
        std::move(map_f32)};
  }
  return metainfo4gui_s{
      archive.metainfo(), false,
      fractal_utils::fractal_map{0, 0, sizeof(std::complex<double>)},
      std::move(map_f32)};
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
                              metainfo->have_mat_z ? &metainfo->mat_z
                                                   : nullptr);
}

void render_fun(const fractal_utils::fractal_map &map_fractal,
                const fractal_utils::wind_base &, void *custom_ptr,
                fractal_utils::fractal_map *map_u8c3) {

  auto *metainfo = reinterpret_cast<metainfo4gui_s *>(custom_ptr);

  for (size_t i = 0; i < map_fractal.element_count(); i++) {
    const float range = metainfo->info.maxit;
    const uint16_t current_age = map_fractal.at<uint16_t>(i);

    if (current_age <= libHybractal::maxit_max) {
      metainfo->mat_f32.at<float>(i) =
          std::clamp<double>(std::sin((current_age / range + M_PI) *
                                      metainfo->num_peroid * 2 * M_PI) /
                                     2 +
                                 0.5,
                             0.0, 1.0);
    } else {
      metainfo->mat_f32.at<float>(i) = 0.0f;
    }
  }

  for (size_t i = 0; i < map_fractal.element_count(); i++) {
    const uint16_t current_age = map_fractal.at<uint16_t>(i);

    if (current_age <= libHybractal::maxit_max) {
      map_u8c3->at<fractal_utils::pixel_RGB>(i) = fractal_utils::color_u8c3(
          metainfo->mat_f32.at<float>(i), fractal_utils::color_series::jet);
    } else {
      map_u8c3->at<fractal_utils::pixel_RGB>(i) = fractal_utils::color_u8c3(
          metainfo->mat_f32.at<float>(i), fractal_utils::color_series::bone);
    }
  }
}