#include <fmt/format.h>
#include <hex_convert.h>

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

std::variant<libHybractal::hybf_ir_new, libHybractal::hybf_metainfo_old>
decode_metainfo(const void *src, size_t bytes, std::string &err) noexcept {
  struct_pack::errc code;
  err.clear();
  {
    libHybractal::hybf_metainfo_old temp;
    code = struct_pack::deserialize_to(temp, (const char *)src, bytes);

    if (code != struct_pack::errc::ok) {
      err += fmt::format(
          "Failed to deserialize to libHybractal::hybf_metainfo_old, detail: "
          "{}\n",
          int64_t(code));
    } else {
      err.clear();
      return temp;
    }
  }

  {
    libHybractal::hybf_ir_new temp;
    code = struct_pack::deserialize_to(temp, (const char *)src, bytes);

    if (code != struct_pack::errc::ok) {
      err += fmt::format("Failed to deserialize to ::hybf_ir_new, detail: "
                         "{}\n",
                         int64_t(code));
    } else {
      err.clear();
      return temp;
    }
  }

  return {};
}

libHybractal::hybf_metainfo_new
libHybractal::hybf_metainfo_new::parse_metainfo(const void *src, size_t bytes,
                                                std::string &err) noexcept {
  auto temp = decode_metainfo(src, bytes, err);

  libHybractal::hybf_metainfo_new ret;
  if (!err.empty()) {
    return {};
  }

  if (std::get_if<libHybractal::hybf_metainfo_old>(&temp) != nullptr) {
    const auto &old = std::get<libHybractal::hybf_metainfo_old>(temp);

    ret.sequence_bin = old.sequence_bin;
    ret.sequence_len = old.sequence_len;
    ret.maxit = old.maxit;
    ret.rows = old.rows;
    ret.cols = old.cols;
    ret.wind.center[0] = old.window_center[0];
    ret.wind.center[1] = old.window_center[1];
    ret.wind.x_span = old.window_xy_span[0];
    ret.wind.y_span = old.window_xy_span[1];
    ret.float_precision = 2;

    ret.chx.resize(4096);

    auto temp = fractal_utils::bin_2_hex(old.window_center.data(),
                                         sizeof(old.window_center),
                                         ret.chx.data(), ret.chx.size(), true);
    ret.chx.resize(temp.value());

    return ret;
  }

  const auto &ir = std::get<hybf_ir_new>(temp);
  ret.sequence_bin = ir.sequence_bin;
  ret.sequence_len = ir.sequence_len;
  ret.maxit = ir.maxit;
  ret.rows = ir.rows;
  ret.cols = ir.cols;
  ret.chx = ir.center_hex;
  ret.float_precision = ir.float_t_prec;
  ret.wind.x_span = ir.window_xy_span[0];
  ret.wind.y_span = ir.window_xy_span[1];

  if (ir.float_t_prec == HYBRACTAL_FLT_PRECISION) {
    auto bytes_opt = fractal_utils::hex_2_bin(
        ir.center_hex, ret.wind.center.data(), sizeof(ret.wind.center));
    if (!bytes_opt.has_value()) {
      err = fmt::format("center_hex is invalid. Center_hex = \"{}\"",
                        ir.center_hex);
      return {};
    }
    if (bytes_opt.value() != sizeof(ret.wind.center)) {
      err = fmt::format(
          "center_hex is invalid. Center_hex = \"{}\",bytes = {}, but expected "
          "{} bytes.",
          ir.center_hex, bytes_opt.value(), sizeof(ret.wind.center));
      return {};
    }
  } else {
    const char *beg = ir.center_hex.c_str();
    if (ir.center_hex.starts_with("0x")) {
      beg += 2;
    }

    const size_t offset_bytes = float_bytes(ir.float_t_prec);
    const size_t offset_chars = offset_bytes * 2;

    const size_t len = strlen(beg);
    if (len != offset_chars * 2) {
      err = fmt::format(
          "Length of center_hex is invalid. center_hex = \"{}\", length = {}, "
          "but expected {} bytes.",
          ir.center_hex, len, offset_chars);
      return {};
    }

    std::string err_cvt[2];

    ret.wind.center[0] =
        any_type_to_compute_t(beg, beg + offset_chars, err_cvt[0]);
    beg += offset_chars;
    ret.wind.center[1] =
        any_type_to_compute_t(beg, beg + offset_chars, err_cvt[1]);

    if (!err_cvt[0].empty() || !err_cvt[1].empty()) {
      err = fmt::format(
          "One or more value in center hex failed to compute. Error 0 = "
          "\"{}\", Error 1 = \"{}\"",
          err_cvt[0], err_cvt[1]);
      return {};
    }
  }

  return ret;
}

libHybractal::hybf_ir_new
libHybractal::hybf_metainfo_new::to_ir() const noexcept {
  hybf_ir_new ir;

  ir.sequence_bin = this->sequence_bin;
  ir.sequence_len = this->sequence_len;
  ir.window_xy_span[0] =
      float_type_cvt<hybf_float_t, double>(this->wind.x_span);
  assert(ir.window_xy_span[0] != 0);
  ir.window_xy_span[1] =
      float_type_cvt<hybf_float_t, double>(this->wind.y_span);
  assert(ir.window_xy_span[1] != 0);

  ir.maxit = this->maxit;
  ir.rows = this->rows;
  ir.cols = this->cols;
  ir.float_t_prec = this->float_precision;

  if (this->chx.empty()) {
    std::string hex;
    hex.resize(4096);
    auto ret = fractal_utils::bin_2_hex(this->wind.center.data(),
                                        sizeof(this->wind.center), hex.data(),
                                        hex.size(), true);

    hex.resize(ret.value());
    ir.center_hex = hex;
  } else {
    ir.center_hex = this->chx;
  }

  return ir;
}

libHybractal::hybf_archive
libHybractal::hybf_archive::load(std::string_view filename,
                                 std::vector<uint8_t> &buffer, std::string *err,
                                 const load_options &opt) noexcept {
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
    result.metainfo() = hybf_metainfo_new::parse_metainfo(
        blkp_meta->data, blkp_meta->bytes, *err);
    if (!err->empty()) {
      *err = fmt::format("Failed to parse metainfo. Detail: {}", *err);
      return {};
    }
  }
  const size_t rows = result.rows();
  const size_t cols = result.cols();

  buffer.reserve(rows * cols * sizeof(std::complex<hybf_store_t>));

  {
    auto blkp_age = bfile.find_block_single(id_mat_age);
    if (blkp_age == nullptr) {
      err->assign("mat_age not found.");
      return {};
    }

    if (opt.compressed_age != nullptr) {
      *opt.compressed_age = buffer;
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

    if (opt.compressed_mat_z != nullptr) {
      opt.compressed_mat_z->clear();
    }

    if (blkp_z != nullptr) {
      if (opt.compressed_mat_z != nullptr) {
        *opt.compressed_mat_z = buffer;
      }

      decompress(blkp_z->data, blkp_z->bytes, buffer);

      if (buffer.size() != rows * cols * sizeof(std::complex<hybf_store_t>)) {
        err->assign(fmt::format(
            "Size of mat_age mismatch. Expected {} but in fact bytes.",
            rows * cols * sizeof(std::complex<hybf_store_t>), buffer.size()));
        return {};
      }
      result.data_z.resize(rows * cols);
      memcpy(result.data_z.data(), buffer.data(), buffer.size());
    }
  }

  if (opt.binfile != nullptr) {
    *opt.binfile = std::move(bfile);
  }

  return result;
}

bool libHybractal::hybf_archive::save(
    std::string_view filename) const noexcept {
  fractal_utils::binfile bfile;

  std::vector<char> meta_info_seralized =
      struct_pack::serialize(this->m_info.to_ir());
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
