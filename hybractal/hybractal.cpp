#include <CLI11.hpp>
#include <hex_convert.h>
#include <libHybfile.h>
#include <libHybractal.h>
#include <omp.h>
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

bool run_compute(const task_compute &task) noexcept;

int main(int argc, char **argv) {
  CLI::App app;

  CLI::App *const compute = app.add_subcommand("compute");
  task_compute task_c;
  compute->add_option("--rows,-r", task_c.info.rows)
      ->required()
      ->check(CLI::PositiveNumber);
  compute->add_option("--cols,-c", task_c.info.cols)
      ->required()
      ->check(CLI::PositiveNumber);
  compute->add_option("--maxit", task_c.info.maxit)
      ->default_val(1024)
      ->check(CLI::Range(uint16_t(1), libHybractal::maxit_max));

  std::string center_hex;

  CLI::Option *const opt_center_double =
      compute->add_option("--center", task_c.info.window_center)
          ->expected(0, 1);
  CLI::Option *const opt_center_hex =
      compute->add_option("--center-hex,--chx", center_hex)->expected(0, 1);

  compute->add_option("--x-span,--span-x", task_c.info.window_xy_span[0])
      ->default_val(-1);
  compute->add_option("--y-span,--span-y", task_c.info.window_xy_span[1])
      ->default_val(2);
  compute->add_option("-o", task_c.filename)->default_val("out.hybf");
  compute->add_option("--threads,-j", task_c.threads)
      ->default_val(std::thread::hardware_concurrency())
      ->check(CLI::PositiveNumber);
  compute->add_flag("--mat-z", task_c.save_mat_z)->default_val(false);

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