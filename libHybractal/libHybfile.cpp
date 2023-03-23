#include <fmt/format.h>

#include <iostream>

#include "libHybfile.h"


libHybractal::hybf_archive::hybf_archive(size_t rows, size_t cols,
                                         bool have_z) {
  this->m_info.rows = rows;
  this->m_info.cols = cols;
  this->data_age.resize(rows * cols);
  if (have_z) {
    this->data_z.resize(rows * cols);
  }
}

#include <zstd.h>

void libHybractal::compress(const void *src, size_t bytes,
                            std::vector<uint8_t> &dest) noexcept {
  dest.clear();
  dest.resize(ZSTD_compressBound(bytes) + 128);

  const size_t size =
      ZSTD_compress(dest.data(), dest.size(), src, bytes, ZSTD_defaultCLevel());

  if (ZSTD_isError(size) || size > dest.capacity()) {
    std::cerr << fmt::format(
                     "compress failed. error code = {}, error name = {}.", size,
                     ZSTD_getErrorName(size))
              << std::endl;
    abort();
    return;
  }

  dest.resize(size);
}

void libHybractal::decompress(const void *src, size_t src_bytes,
                              std::vector<uint8_t> &dest) noexcept {
  const size_t dst_bytes = ZSTD_getDecompressedSize(src, src_bytes);
  dest.resize(dst_bytes);
  const size_t capacity = dest.size();
  const size_t ret = ZSTD_decompress(dest.data(), capacity, src, src_bytes);
  if (ZSTD_isError(ret) || ret > capacity) {
    std::cerr << fmt::format(
                     "compress failed. error code = {}, error name = {}.", ret,
                     ZSTD_getErrorName(ret))
              << std::endl;
    abort();
    return;
  }

  assert(ret <= capacity);

  dest.resize(ret);
}

std::vector<uint8_t> libHybractal::compress(const void *src,
                                            size_t bytes) noexcept {
  std::vector<uint8_t> result;
  compress(src, bytes, result);
  return result;
}

#include <fractal_binfile.h>

#include <fstream>
#include <iostream>
#include <struct_pack/struct_pack.hpp>
#include <vector>


enum seg_id : int64_t {
  id_metainfo = 666,
  id_mat_age = 114514,
  id_mat_z = 1919810,
};

libHybractal::hybf_archive libHybractal::hybf_archive::load(
    std::string_view filename, std::vector<uint8_t> &buffer,
    std::string *err) noexcept {
  fractal_utils::binfile bfile;

  if (!bfile.parse_from_file(filename.data())) {
    err->assign(fmt::format("Failed to parse {}.", filename));
    return {};
  }
  hybf_archive result;
  {
    auto blkp_meta = bfile.find_block_single(id_metainfo);
    if (blkp_meta == nullptr) {
      err->assign("metadata not found.");
      return {};
    }
    struct_pack::errc error_code = struct_pack::deserialize_to(
        result.m_info, (const char *)blkp_meta->data, blkp_meta->bytes);

    if (error_code != struct_pack::errc::ok) {
      err->assign(fmt::format("deserialization failed. error code = {}",
                              (int64_t)error_code));
      return {};
    }
  }
  const size_t rows = result.rows();
  const size_t cols = result.cols();

  buffer.reserve(rows * cols * sizeof(std::complex<double>));

  {
    auto blkp_age = bfile.find_block_single(id_mat_age);
    if (blkp_age == nullptr) {
      err->assign("mat_age not found.");
      return {};
    }

    decompress(blkp_age->data, blkp_age->bytes, buffer);

    if (buffer.size() != rows * cols * sizeof(uint16_t)) {
      err->assign(fmt::format(
          "Size of mat_age mismatch. Expected {} but in fact bytes.",
          rows * cols * sizeof(uint16_t), buffer.size()));
      return {};
    }

    result.data_age.resize(result.rows() * result.cols());
    memcpy(result.data_age.data(), buffer.data(), buffer.size());
  }

  {
    auto blkp_z = bfile.find_block_single(id_mat_z);
    if (blkp_z != nullptr) {
      decompress(blkp_z->data, blkp_z->bytes, buffer);

      if (buffer.size() != rows * cols * sizeof(std::complex<double>)) {
        err->assign(fmt::format(
            "Size of mat_age mismatch. Expected {} but in fact bytes.",
            rows * cols * sizeof(std::complex<double>), buffer.size()));
        return {};
      }
      result.data_z.resize(rows * cols);
      memcpy(result.data_z.data(), buffer.data(), buffer.size());
    }
  }

  return result;
}

bool libHybractal::hybf_archive::save(
    std::string_view filename) const noexcept {
  fractal_utils::binfile bfile;

  std::vector<char> meta_info_seralized = struct_pack::serialize(this->m_info);
  bfile.blocks.emplace_back(
      fractal_utils::data_block{id_metainfo, meta_info_seralized.size(), 0,
                                meta_info_seralized.data(), false});

  auto compressed_age{compress(this->data_age.data(),
                               this->data_age.size() * sizeof(uint16_t))};
  bfile.blocks.emplace_back(fractal_utils::data_block{
      id_mat_age, compressed_age.size(), 0, compressed_age.data(), false});

  std::vector<uint8_t> compressed_z{};

  if (this->have_mat_z()) {
    compressed_z = compress(this->data_z.data(),
                            this->data_z.size() * sizeof(this->data_z[0]));
    bfile.blocks.emplace_back(fractal_utils::data_block{
        id_mat_z, compressed_z.size(), 0, compressed_z.data(), false});
  }

  return bfile.save_to_file(filename.data(), true);
}
