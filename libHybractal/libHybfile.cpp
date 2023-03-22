#include "libHybfile.h"

#include <fmt/format.h>

libHybractal::hybf_file::hybf_file(size_t __rows, size_t __cols, bool has_mat_z)
    : mat_age{__rows, __cols, sizeof(uint16_t)}, mat_z(std::nullopt) {
  if (has_mat_z) {
    this->mat_z.emplace(fractal_utils::fractal_map(
        __rows, __cols, sizeof(std::complex<double>)));
  }
}

#include <lz4.h>

void libHybractal::compress_lz4(const void *src, size_t bytes,
                                std::vector<uint8_t> &dest) noexcept {
  dest.clear();
  dest.reserve(bytes + 128);

  while (true) {

    const int size = LZ4_compress_default(
        (const char *)src, (char *)dest.data(), bytes, dest.capacity());

    if (size <= 0) {
      dest.reserve(dest.capacity() * 2);
      continue;
    } else {
      dest.resize(size);
      break;
    }
  }
}

void libHybractal::decompress_lz4(const void *src, size_t src_bytes,
                                  std::vector<uint8_t> &dest) noexcept {
  while (true) {
    const int size = LZ4_decompress_safe((const char *)src, (char *)dest.data(),
                                         src_bytes, dest.capacity());
    if (size <= 0) {
      dest.reserve(dest.capacity() * 4);
      continue;
    } else {
      dest.resize(size);
      break;
    }
  }
}

std::vector<uint8_t> libHybractal::compress_lz4(const void *src,
                                                size_t bytes) noexcept {
  std::vector<uint8_t> result;
  compress_lz4(src, bytes, result);
  return result;
}

#include <fractal_binfile.h>
#include <fstream>
#include <iostream>
#include <struct_pack/struct_pack.hpp>
#include <vector>

enum seg_id : int64_t {
  id_metainfo = 0,
  id_mat_age = 114514,
  id_mat_z = 1919810,
};

struct info_for_serial_s {
  libHybractal::hybf_metainfo info;
  size_t rows;
  size_t cols;
};

const fractal_utils::data_block *find_block(const fractal_utils::binfile &bf,
                                            seg_id tag) noexcept {

  for (const auto &blk : bf.blocks) {
    if (blk.tag == tag) {
      return &blk;
    }
  }

  return nullptr;
}

libHybractal::hybf_file
libHybractal::hybf_file::load(std::string_view filename,
                              std::string *err) noexcept {

  fractal_utils::binfile binfile;

  if (!binfile.parse_from_file(filename.data())) {
    err->assign("binfile.parse_from_file failed.");
    return {0, 0, false};
  }

  libHybractal::hybf_metainfo mtif;
  size_t rows, cols;
  {
    auto blkp_info = find_block(binfile, id_metainfo);
    if (blkp_info == nullptr) {
      err->assign("metadata not found.");
      return {0, 0, false};
    }

    info_for_serial_s ifss;
    struct_pack::errc errcode = struct_pack::deserialize_to(
        ifss, (const char *)blkp_info->data, blkp_info->bytes);

    if (errcode != struct_pack::errc::ok) {
      err->assign("struct_pack failed to parse.");
      return {0, 0, false};
    }

    mtif = ifss.info;

    rows = ifss.rows;
    cols = ifss.cols;

    if (rows <= 0 || cols <= 0) {
      err->assign(fmt::format("rows = {}, cols = {}", rows, cols));
      return {0, 0, false};
    }
  }

  fractal_utils::fractal_map map_age(rows, cols, sizeof(uint16_t));

  {
    const auto *blkp_age = find_block(binfile, seg_id::id_mat_age);

    if (blkp_age == nullptr) {
      err->assign("Failed to find mat_age");
      return {0, 0, false};
    }

    std::vector<uint8_t> buffer;
    buffer.reserve(rows * cols * sizeof(std::complex<double>));

    decompress_lz4(blkp_age->data, blkp_age->bytes, buffer);

    if (buffer.size() != rows * cols * sizeof(uint16_t)) {
      err->assign(fmt::format(
          "mat_age size mismatch. Expected {} bytes, but infact {} bytes",
          rows * cols * sizeof(uint16_t), buffer.size()));
      return {0, 0, false};
    }
    memcpy(map_age.data, buffer.data(), map_age.byte_count());
  }

#warning here
}

bool libHybractal::hybf_file::save(std::string_view filename) const noexcept {

  fractal_utils::binfile binfile;

  binfile.callback_free = nullptr;
  binfile.callback_malloc = nullptr;

  std::vector<char> data_metainfo;
  {
    info_for_serial_s ifss{this->info, this->rows(), this->cols()};

    struct_pack::serialize_to(data_metainfo, ifss);
  }

  binfile.blocks.emplace_back(fractal_utils::data_block{
      id_metainfo, data_metainfo.size(), 0, data_metainfo.data()});

  std::vector<uint8_t> data_age =
      compress_lz4(this->mat_age.data, this->mat_age.byte_count());
  {
    binfile.blocks.emplace_back(fractal_utils::data_block{
        id_mat_age, data_age.size(), 0, data_age.data()});
  }

  std::vector<uint8_t> data_mat_z;
  if (this->have_z_mat()) {
    const auto &ref = this->mat_z.value();
    data_mat_z = compress_lz4(ref.data, ref.byte_count());
    binfile.blocks.emplace_back(fractal_utils::data_block{
        id_mat_z, data_mat_z.size(), 0, data_mat_z.data()});
  }

  return fractal_utils::serialize_to_file(
      binfile.blocks.data(), binfile.blocks.size(), true, filename.data());
}