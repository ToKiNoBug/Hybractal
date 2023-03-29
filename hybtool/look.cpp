/*
 Copyright © 2023  TokiNoBug
This file is part of Hybractal.

    Hybractal is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Hybractal is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Hybractal.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "hybtool.h"
#include <fmt/format.h>
#include <fstream>
#include <hex_convert.h>
#include <iostream>

using std::cout, std::cerr, std::endl;

bool export_bin_file(std::string_view filename, const void *data,
                     size_t bytes) noexcept;

bool run_look(const task_look &task) noexcept {
  libHybractal::load_options opt;

  fractal_utils::binfile binfile;

  opt.binfile = &binfile;

  std::string err{""};

  libHybractal::hybf_archive archive =
      libHybractal::hybf_archive::load(task.file, &err, opt);

  if (!err.empty()) {
    cerr << fmt::format("Failed to load hybf file \"{}\". Detail: {}",
                        task.file, err)
         << endl;
    return false;
  }

  if (task.show_blocks || task.show_all) {
    cout << "Data blocks: \n";
    size_t idx = 0;
    for (const auto &blk : binfile.blocks) {
      cout << fmt::format("    Block {}: tag = {}, size = {}, offset = {}\n",
                          idx, blk.tag, blk.bytes, blk.file_offset);
    }
    cout << endl;
  }

  const auto &metainfo = archive.metainfo();
  if (task.show_sequence || task.show_all) {
    std::string seq_str =
        fmt::format("{:{}b}", metainfo.sequence_bin, metainfo.sequence_len);
    cout << fmt::format("Sequence: sequence = \"{}\", length = {}\n", seq_str,
                        metainfo.sequence_len);
    cout << endl;
  }

  if (task.show_size || task.show_all) {
    cout << fmt::format("Size: rows = {}, cols = {}, pixel num = {}\n",
                        metainfo.rows, metainfo.cols,
                        metainfo.rows * metainfo.cols);
    cout << endl;
  }

  if (task.show_window || task.show_all) {
    cout << fmt::format(
        "Window: center = {}{:+}i, x_span = {}, y_span = {}\n",
        double(metainfo.wind.center[0]), double(metainfo.wind.center[1]),
        double(metainfo.wind.x_span), double(metainfo.wind.y_span));
    cout << endl;
  }

  if (task.show_center_hex || task.show_all) {
    cout << fmt::format("Center hex: {},\n length = {}\n",
                        metainfo.center_hex(), metainfo.center_hex().size());
    cout << endl;
  }

  if (task.show_maxit) {
    cout << fmt::format("Maxit : {}\n", metainfo.maxit);
    cout << endl;
  }

  if (task.show_precision || task.show_all) {
    cout << fmt::format("Floating point precision : {}\n",
                        metainfo.float_precision);
    cout << endl;
  }

  const auto blk_age =
      binfile.find_block_single(libHybractal::hybf_archive::seg_id::id_mat_age);

  if (blk_age == nullptr) {
    cerr << fmt::format(
                "Fatal error : failed to find data block containing mat_age.")
         << endl;
    return false;
  }

  if (!task.extract_age_compressed.empty()) {
    if (!export_bin_file(task.extract_age_compressed, blk_age->data,
                         blk_age->bytes)) {
      return false;
    }
  }

  if (!task.extract_age_decompress.empty()) {
    if (!export_bin_file(task.extract_age_decompress,
                         archive.mat_age_data().data(),
                         archive.mat_age_data().size() *
                             sizeof(archive.mat_age_data()[0]))) {
      return false;
    }
  }

  const auto blk_z =
      binfile.find_block_single(libHybractal::hybf_archive::seg_id::id_mat_z);

  if (!task.extract_z_compressed.empty()) {
    if (blk_z == nullptr) {
      cerr
          << "Trying to extract compress z mat, but no z mat data blocks found."
          << endl;
      return false;
    }
    if (!export_bin_file(task.extract_z_compressed, blk_z->data,
                         blk_z->bytes)) {
      return false;
    }
  }

  if (!task.extract_z_decompress.empty()) {
    if (!archive.have_mat_z()) {
      cerr << "Trying to extract compress z mat, but archive.have_mat_z() "
              "returns false."
           << endl;
      return false;
    }
    if (!export_bin_file(task.extract_z_decompress, archive.mat_z_data().data(),
                         archive.mat_z_data().size() *
                             sizeof(archive.mat_z_data()[0]))) {
      return false;
    }
  }

  return true;
}

bool export_bin_file(std::string_view filename, const void *data,
                     size_t bytes) noexcept {
  try {
    std::ofstream ofs{filename.data(), std::ios::binary};
    ofs.write((const char *)data, bytes);
    ofs.close();
  } catch (std::exception &e) {
    cerr << fmt::format("Failed to export {}, detail: {}", filename, e.what())
         << endl;
    return false;
  }
  return true;
}