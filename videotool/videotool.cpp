#include <CLI11.hpp>
#include <iostream>

#include "videotool.h"

int main(int argc, char **argv) {
  CLI::App app;

  app.set_version_flag("--version,-v", HYBRACTAL_VERSION);

  std::string taskfile;
  app.add_option("taskfile", taskfile, "Json task file")
      ->required()
      ->check(CLI::ExistingFile)
      ->check(CLI::Validator{[](std::string &str) -> std::string {
                               if (str.ends_with(".json")) {
                                 return "";
                               }
                               return "Not a json file";
                             },
                             "Json"},
              "Json file");

  auto compute = app.add_subcommand("compute");
  auto render = app.add_subcommand("render");
  auto mkvideo = app.add_subcommand("makevideo");

  bool dry_run{false};
  mkvideo->add_flag("--dry-run", dry_run, "Print commands instead of execute.")
      ->default_val(false);

  CLI11_PARSE(app, argc, argv);

  auto taskf_opt = load_task(taskfile);

  if (!taskf_opt.has_value()) {
    std::cerr << "Failed to load " << taskfile << std::endl;
    return 1;
  }

  auto &taskf = taskf_opt.value();

  if (taskf.compute.x_span <= 0) {
    taskf.compute.x_span =
        taskf.compute.y_span * taskf.common.rows / taskf.common.cols;
  }

  if (compute->count() > 0) {
    if (!run_compute(taskf.common, taskf.compute)) {
      std::cerr << "Computation terminated with error." << std::endl;
      return 1;
    }
  }

  if (render->count() > 0) {
    if (!run_render(taskf.common, taskf.render)) {
      std::cerr << "Render terminated with error." << std::endl;
      return 1;
    }
  }

  if (mkvideo->count() > 0) {
    if (!run_makevideo(taskf.common, taskf.render, taskf.video, dry_run)) {
      std::cerr << "Makevideo terminated with error." << std::endl;
      return 1;
    }
  }

  std::cout << "Success" << std::endl;

  return 0;
}