#include <fmt/format.h>
#include <hex_convert.h>
#include <libHybfile.h>
#include <omp.h>

#include <filesystem>
#include <iostream>

#include "videotool.h"

using std::cout, std::cerr, std::endl;

std::vector<int> unfinished_tasks(const common_info &common,
                                  const compute_task &ct) noexcept;

bool run_compute(const common_info &common,
                 const compute_task &ctask) noexcept {
  omp_set_num_threads(ctask.threads);

  libHybractal::hybf_archive archive(common.rows, common.cols, true);

  archive.metainfo().maxit = common.maxit;

  {
    std::string err;
    archive.metainfo().wind = libHybractal::make_center_wind_variant(
        ctask.center_hex, ctask.x_span, ctask.y_span, ctask.precision, false,
        err);

    if (!err.empty()) {
      cerr << fmt::format("Invalid center hex. Detail: {}", err) << endl;
      return false;
    }
  }

  const auto frame_idxs = unfinished_tasks(common, ctask);
  const int task_num = frame_idxs.size();

  int counter = 0;
  for (int fidx : frame_idxs) {
    const double factor = std::pow(common.ratio, -fidx);

    auto update_xy_span = [ctask, factor](auto &wind) {
      wind.x_span = ctask.x_span * factor;
      wind.y_span = ctask.y_span * factor;
    };

    std::visit(update_xy_span, archive.metainfo().wind);

    cout << endl;
    std::string filename = hybf_filename(common, fidx);
    cout << fmt::format("[{:^6.1f}% : {:^3} / {:^3}] : {}",
                        100 * float(counter) / task_num, counter, task_num,
                        filename);

    auto mat_age = archive.map_age();
    auto mat_z = archive.map_z();

    ::libHybractal::compute_frame_by_precision(archive.metainfo().window_base(),
                                               archive.metainfo().precision(),
                                               common.maxit, mat_age, &mat_z);

    const bool ok = archive.save(filename);

    if (!ok) {
      cerr << fmt::format("\nFailed to export hybf file: {}\n", filename);
      return false;
    }
    counter++;
  }

  cout << fmt::format("\n[{:^6.1f}% : {:^3} / {:^3}] : All tasks finished.",
                      100.0f, counter, task_num)
       << endl;

  return true;
}

bool check_hybf(std::string_view filename, const common_info &ci,
                std::vector<uint8_t> &buffer, bool &exist,
                const check_hybf_option &opt) noexcept {
  if (!std::filesystem::is_regular_file(filename)) {
    exist = false;
    return false;
  }
  exist = true;

  std::string err;
  libHybractal::hybf_archive hybf_archive =
      libHybractal::hybf_archive::load(filename, buffer, &err);

  if (!err.empty()) {
    cout << fmt::format(
        "Warning: {} exists, but can not be loaded(detail: {}). This is "
        "considered to be not-computed. This file will be "
        "deleted and compute again.",
        filename, err);
    return false;
  }

  if (!opt.nocheck_sequence) {
    if ((hybf_archive.metainfo().sequence_bin !=
         ::libHybractal::global_sequence_bin) ||
        hybf_archive.metainfo().sequence_len !=
            ::libHybractal::global_sequence_len) {
      cout << fmt::format(
          "Warning: {} is loaded as a hybf file, but the sequence({:{}b}) "
          "mismatch with this program's configuration({}).",
          filename, hybf_archive.metainfo().sequence_bin,
          hybf_archive.metainfo().sequence_len, HYBRACTAL_SEQUENCE_STR);
      return false;
    }
  }

  if (!opt.ignore_size) {
    if (!check_hybf_size(hybf_archive, {ci.rows, ci.cols})) {
      cout
          << fmt::format(
                 "Warning: {} is loaded as a hybf file, but the size([{}, {}]) "
                 "mismatch with task([{}, {}])",
                 filename, hybf_archive.metainfo().rows,
                 hybf_archive.metainfo().cols, ci.rows, ci.cols)
          << endl;
      return false;
    }
  }

  if (opt.move_archive != nullptr) {
    *opt.move_archive = std::move(hybf_archive);
  }

  return true;
}

bool check_hybf_size(const libHybractal::hybf_archive &archive,
                     const std::array<size_t, 2> &expected_size) noexcept {
  if (archive.metainfo().rows != expected_size[0]) {
    return false;
  }
  if (archive.metainfo().cols != expected_size[1]) {
    return false;
  }

  return true;
}

std::vector<int> unfinished_tasks(const common_info &common,
                                  const compute_task &ct) noexcept {
  std::vector<uint8_t> need_compute;
  need_compute.resize(common.frame_num);
  for (int fidx = 0; fidx < common.frame_num; fidx++) {
    need_compute[fidx] = true;
  }

#pragma omp parallel for schedule(dynamic)
  for (int fidx = 0; fidx < common.frame_num; fidx++) {
    thread_local std::vector<uint8_t> buffer;

    bool exist;
    std::string filename = hybf_filename(common, fidx);
    if (check_hybf(filename, common, buffer, exist)) {
      need_compute[fidx] = false;
      continue;
    }

    if (exist) {
      const bool ok = std::filesystem::remove(filename);

      if (!ok) {
        cerr << "Failed to delete " << filename << endl;
      }
    }
  }

  std::vector<int> ret;
  ret.reserve(common.frame_num);

  for (int fidx = 0; fidx < common.frame_num; fidx++) {
    if (need_compute[fidx]) {
      ret.emplace_back(fidx);
    }
  }

  return ret;
}