#include "videotool.h"
#include <filesystem>
#include <fmt/format.h>
#include <hex_convert.h>
#include <iostream>
#include <libHybfile.h>
#include <omp.h>

using std::cout, std::cerr, std::endl;

bool check_hybf(std::string_view filename, std::vector<uint8_t> &buffer,
                bool &do_delete) noexcept;

std::vector<int> unfinished_tasks(const common_info &common,
                                  const compute_task &ct) noexcept;

bool run_compute(const common_info &common,
                 const compute_task &ctask) noexcept {
  omp_set_num_threads(ctask.threads);

  libHybractal::hybf_archive archive(common.rows, common.cols, true);

  archive.metainfo().maxit = common.maxit;
  archive.metainfo().window_xy_span = {ctask.x_span, ctask.y_span};

  {
    auto bytes = fractal_utils::hex_2_bin(
        ctask.center_hex, archive.metainfo().window_center.data(),
        sizeof(archive.metainfo().window_center));
    if (!bytes.has_value() ||
        bytes.value() != sizeof(archive.metainfo().window_center)) {
      cerr << fmt::format("Invalid center hex.") << endl;
      return false;
    }
  }

  const auto frame_idxs = unfinished_tasks(common, ctask);
  const int task_num = frame_idxs.size();

  int counter = 0;
  for (int fidx : frame_idxs) {
    const double factor = std::pow(ctask.ratio, fidx);

    archive.metainfo().window_xy_span[0] = ctask.x_span * factor;
    archive.metainfo().window_xy_span[1] = ctask.y_span * factor;

    std::string filename = hybf_filename(common, fidx);
    cout << fmt::format("[{}% : {} / {}] : {}", float(counter) / task_num,
                        counter, task_num, filename)
         << endl;

    auto mat_age = archive.map_age();
    auto mat_z = archive.map_z();

    ::libHybractal::compute_frame(archive.metainfo().window(), common.maxit,
                                  mat_age, &mat_z);

    const bool ok = archive.save(filename);

    if (!ok) {
      cerr << fmt::format("\nFailed to export hybf file: {}\n", filename);
      return false;
    }
    counter++;
  }

  cout << fmt::format("[{}% : {} / {}] : All tasks finished.",
                      float(counter) / std::max(1, task_num), counter, task_num)
       << endl;

  return true;
}

bool check_hybf(std::string_view filename, std::vector<uint8_t> &buffer,
                bool &do_delete) noexcept {
  do_delete = false;
  if (!std::filesystem::is_regular_file(filename)) {
    return false;
  }

  std::string err;
  libHybractal::hybf_archive hybf_archive =
      libHybractal::hybf_archive::load(filename, buffer, &err);

  if (!err.empty()) {
    cout << fmt::format(
        "Warning: {} exists, but can not be loaded(detail: {}). This is "
        "considered to be not-computed. This file will be "
        "deleted and compute again.",
        filename, err);
    do_delete = true;
    return false;
  }

  if ((hybf_archive.metainfo().sequence_bin !=
       ::libHybractal::global_sequence_bin) ||
      hybf_archive.metainfo().sequence_len !=
          ::libHybractal::global_sequence_len) {
    cout << fmt::format("Warning: {} is a hybf file, but the sequence(\{:{}b}) "
                        "mismatch with this program's configuration({}).",
                        filename, hybf_archive.metainfo().sequence_bin,
                        hybf_archive.metainfo().sequence_len,
                        HYBRACTAL_SEQUENCE_STR);
    do_delete = true;
    return false;
  }

  return true;
}

std::vector<int> unfinished_tasks(const common_info &common,
                                  const compute_task &ct) noexcept {

  std::vector<uint8_t> need_compute;
  need_compute.resize(ct.frame_num);
  for (int fidx = 0; fidx < ct.frame_num; fidx++) {

    need_compute[fidx] = true;
  }

#pragma omp parallel for schedule(dynamic)
  for (int fidx = 0; fidx < ct.frame_num; fidx++) {
    thread_local std::vector<uint8_t> buffer;

    bool do_delete;
    std::string filename = hybf_filename(common, fidx);
    if (check_hybf(filename, buffer, do_delete)) {
      need_compute[fidx] = false;
      continue;
    }

    if (do_delete) {
      const bool ok = std::filesystem::remove(filename);

      if (!ok) {
        cerr << "Failed to delete " << filename << endl;
      }
    }
  }

  std::vector<int> ret;
  ret.reserve(ct.frame_num);

  for (int fidx = 0; fidx < ct.frame_num; fidx++) {
    if (need_compute[fidx]) {
      ret.emplace_back(fidx);
    }
  }

  return ret;
}