#include "videotool.h"

#include <CLI11.hpp>
#include <iostream>

int main(int argc, char **argv) {
  CLI::App app;
  std::string taskfile;
  app.add_option("taskfile", taskfile, "Json task file")
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

  CLI11_PARSE(app, argc, argv);

  auto taskv_opt = load_video_task(taskfile);

  if (!taskv_opt.has_value()) {
    std::cerr << "Failed to load " << taskfile << std::endl;
    return 1;
  }

  auto &taskv = taskv_opt.value();

  if (taskv.compute.x_span <= 0) {
    taskv.compute.x_span =
        taskv.compute.y_span * taskv.common.rows / taskv.common.cols;
  }

  return 0;
}