#include <CLI11.hpp>
#include <fmt/format.h>
#include <iostream>
#include <libHybfile.h>

using std::cout, std::cerr, std::endl;

int load(std::string_view filename) noexcept;

int main(int argc, char **argv) {

  CLI::App app;

  std::string filename{""};

  app.add_option("file", filename);

  CLI11_PARSE(app, argc, argv);

  if (load(filename)) {
    cerr << "Failed." << endl;
    return 1;
  }

  cout << "Success" << endl;
  return 0;
}

int load(std::string_view filename) noexcept {
  std::string err;
  libHybractal::hybf_archive file =
      libHybractal::hybf_archive::load(filename, &err);

  if (!err.empty()) {
    cerr << fmt::format("Failed to load {}, detail : {}", filename, err)
         << endl;
    return 1;
  }

  cout << fmt::format("{}  meta info:\n", filename);

  cout << fmt::format("    rows = {}, cols = {}\n", file.rows(), file.cols());
  cout << fmt::format("    sequence = {:b}, length = {}\n",
                      file.metainfo().sequence_bin,
                      file.metainfo().sequence_len);

  return 0;
}