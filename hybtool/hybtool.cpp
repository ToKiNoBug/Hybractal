#include <fmt/format.h>
#include <hex_convert.h>
#include <libHybfile.h>
#include <libHybractal.h>
#include <omp.h>

#include <CLI11.hpp>
#include <thread>
#include <variant>

struct task_compute {
  libHybractal::hybf_metainfo info;
  std::string filename;
  uint16_t threads{1};
  bool save_mat_z{false};
  void override_x_span() noexcept {
    this->info.window_xy_span[0] =
        this->info.window_xy_span[1] * info.cols / info.rows;
  }

  void override_center(const std::array<uint64_t, 2> &src) noexcept;
};

struct task_render {
  std::string json_file;
  std::string png_file;
  std::string hybf_file;
};

bool run_compute(const task_compute &task) noexcept;

bool run_render(const task_render &task) noexcept;

int main(int argc, char **argv) {
  CLI::App app;

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

  CLI11_PARSE(app, argc, argv);

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

  std::cout << "Success" << std::endl;

  return 0;
}

bool run_compute(const task_compute &task) noexcept {
  omp_set_num_threads(task.threads);

  libHybractal::hybf_archive file(task.info.rows, task.info.cols,
                                  task.save_mat_z);

  file.metainfo() = task.info;
  fractal_utils::fractal_map mat_age = file.map_age();
  fractal_utils::fractal_map mat_z = file.map_z();
  libHybractal::compute_frame(file.metainfo().window(), file.metainfo().maxit,
                              mat_age, file.have_mat_z() ? &mat_z : nullptr);

  return file.save(task.filename);
}

#include <png_utils.h>

#include "libRender.h"

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

  libHybractal::render_hsv(src.map_age(), src.map_z(), img_u8c3,
                           render_opt.value(), gpu_rcs);

  const bool ok = fractal_utils::write_png(
      task.png_file.c_str(), fractal_utils::color_space::u8c3, img_u8c3);

  if (!ok) {
    std::cout << "Failed to export png" << std::endl;
    return false;
  }

  return true;
}