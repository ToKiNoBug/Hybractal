#include <CLI11.hpp>
#include <libHybfile.h>
#include <libHybractal.h>

int main(int argc, char **argv) {

  size_t rows{0}, cols{0};
  fractal_utils::center_wind<double> wind;

  bool save_mat_z{false};

  CLI::App app;
  app.add_option("--rows", rows)->required();
  app.add_option("--cols", cols)->required();

  app.add_option("--center", wind.center)->required();

  app.add_option("--xspan", wind.x_span)->default_val(2);
  app.add_option("--yspan", wind.y_span)->default_val(2);

  app.add_flag("--mat-z", save_mat_z)->default_val(false);

  CLI11_PARSE(app, argc, argv);

  libHybractal::hybf_file hf{rows, cols, save_mat_z};

  if (!hf.save("test.hybf")) {
    std::cout << "Failed to serialize" << std::endl;
    return 1;
  }

  std::cout << "Success" << std::endl;

  return 0;
}