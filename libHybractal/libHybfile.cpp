#include <fmt/format.h>
#include <hex_convert.h>

#include <iostream>

#include "float_encode.hpp"
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

libHybractal::hybf_ir_new private_fun_decode_ir(const void *src, size_t bytes,
                                                std::string &err) noexcept {

  libHybractal::hybf_ir_new temp;
  struct_pack::errc code;
  code = struct_pack::deserialize_to(temp, (const char *)src, bytes);

  if (code != struct_pack::errc::ok) {
    err += fmt::format("Failed to deserialize to ::hybf_ir_new, detail: "
                       "{}\n",
                       int64_t(code));
  } else {
    err.clear();
    return temp;
  }

  return {};
}

std::variant<libHybractal::hybf_ir_new, libHybractal::hybf_metainfo_old>
decode_metainfo(const void *src, size_t bytes, std::string &err) noexcept {
  err.clear();
  {
    struct_pack::errc code;
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

  return private_fun_decode_ir(src, bytes, err);
}

#define HYBFILE_make_center_wind_variant_MAKE_CASE(precision)                  \
  case precision: {                                                            \
    fractal_utils::center_wind<float_by_prec_t<precision>> temp;               \
    if (bytes != sizeof(temp.center)) {                                        \
      err = fmt::format(                                                       \
          "The bytes of centerhex mismatch with center. Expected {} bytes, "   \
          "but actually {} bytes.",                                            \
          sizeof(temp.center), bytes);                                         \
      return {};                                                               \
    }                                                                          \
    memcpy(temp.center.data(), buffer, sizeof(temp.center));                   \
    temp.x_span =                                                              \
        libHybractal::float_type_cast<float_by_prec_t<precision>>::cast(       \
            x_span);                                                           \
    temp.y_span =                                                              \
        libHybractal::float_type_cast<float_by_prec_t<precision>>::cast(       \
            y_span);                                                           \
    return temp;                                                               \
  }

libHybractal::center_wind_variant_t
libHybractal::make_center_wind_variant(std::string_view chx, double x_span,
                                       double y_span, int precision,
                                       bool is_old, std::string &err) noexcept {
  err.clear();
  if (chx.starts_with("0x") || chx.starts_with("0X")) {
    return make_center_wind_variant({chx.begin() + 2, chx.end()}, x_span,
                                    y_span, precision, is_old, err);
  }

  if (!libHybractal::is_valid_precision(precision)) {
    err = fmt::format("Invalid precision {}.", precision);
    return {};
  }

  if (is_old) {
    const size_t required_chars = libHybractal::float_bytes(precision) * 2 * 2;
    if (chx.length() != required_chars) {
      err = fmt::format(
          "Length of center_hex is invalid. center_hex = \"{}\", length = {}, "
          "but expected {} chars. Precision = {}",
          chx, chx.length(), required_chars, precision);
      return {};
    }

    uint8_t buffer[4096];
    auto bytes_opt = fractal_utils::hex_2_bin(chx, buffer, sizeof(buffer));

    if (!bytes_opt.has_value()) {
      err = fmt::format(
          "Failed to decode center hex into binary, the center hex exceeds {} "
          "bytes. This length is impossible.",
          sizeof(buffer));
      return {};
    }

    size_t bytes = bytes_opt.value();

    switch (precision) {
      HYBFILE_make_center_wind_variant_MAKE_CASE(1);
      HYBFILE_make_center_wind_variant_MAKE_CASE(2);
      HYBFILE_make_center_wind_variant_MAKE_CASE(4);
      HYBFILE_make_center_wind_variant_MAKE_CASE(8);
    default:
      abort();
    }

    return {};
  }

  // it must not be old. decode as ieee

  const size_t bytes_expected = 2 * precision * sizeof(float);
  const size_t chars_expected = 2 * bytes_expected;

  if (chx.length() != chars_expected) {
    err = fmt::format("Hex string length mismatch. Expected {} chars({} "
                      "bytes), but actually {} chars.",
                      chars_expected, bytes_expected, chx.length());
    return {};
  }

  uint8_t buffer[4096];

  auto center_bytes_opt = fractal_utils::hex_2_bin(chx, buffer, sizeof(buffer));
  if (!center_bytes_opt.has_value()) {
    err = fmt::format("The center hex string is too long(exceeds {} bytes).",
                      sizeof(buffer));
    return {};
  }

  const size_t center_bytes = center_bytes_opt.value();

  libHybractal::center_wind_variant_t ret;

  switch (precision) {
  case 1:
    ret = fractal_utils::center_wind<float_by_prec_t<1>>{};
    break;
  case 2:
    ret = fractal_utils::center_wind<float_by_prec_t<2>>{};
    break;
  case 4:
    ret = fractal_utils::center_wind<float_by_prec_t<4>>{};
    break;
  case 8:
    ret = fractal_utils::center_wind<float_by_prec_t<8>>{};
    break;
  }

  auto set_wind = [buffer, center_bytes, &err, x_span, y_span](auto &wind) {
    using flt_t = std::decay_t<decltype(wind.center[0])>;

    auto temp = decode_array2<flt_t>(buffer, center_bytes);

    if (!temp.has_value()) {
      err = fmt::format("Failed to decode center hex.");
      return;
    }
    wind.x_span = flt_t(x_span);
    wind.y_span = flt_t(y_span);
  };

  std::visit(set_wind, ret);

  return ret;
}

libHybractal::hybf_metainfo_new
libHybractal::hybf_metainfo_new::parse_metainfo_gen0(
    const void *src, size_t bytes, std::string &err) noexcept {
  auto temp = decode_metainfo(src, bytes, err);

  libHybractal::hybf_metainfo_new ret;
  ret.file_genration = 0;

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

    ret.wind = make_center_wind_by_prec<2>(
        {old.window_center[0], old.window_center[1]}, old.window_xy_span[0],
        old.window_xy_span[1]);

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
  {
    std::string mcwv_err{};
    ret.wind = make_center_wind_variant(ir.center_hex, ir.window_xy_span[0],
                                        ir.window_xy_span[1], ir.float_t_prec,
                                        true, mcwv_err);

    if (!mcwv_err.empty()) {
      err = fmt::format("Failed to parse centerhex and xy span. Detail: {}",
                        mcwv_err);
      return {};
    }
  }

  return ret;
}

libHybractal::hybf_metainfo_new
libHybractal::hybf_metainfo_new::parse_metainfo_gen1(
    const void *src, size_t bytes, std::string &err) noexcept {
  err.clear();
  auto ir = private_fun_decode_ir(src, bytes, err);

  if (!err.empty()) {
    err = fmt::format("Failed to decode ir. Detail: {}.", err);
    return {};
  }

  hybf_metainfo_new ret;
  ret.file_genration = 1;

  ret.sequence_bin = ir.sequence_bin;
  ret.sequence_len = ir.sequence_len;
  ret.maxit = ir.maxit;
  ret.rows = ir.rows;
  ret.cols = ir.cols;
  ret.chx = ir.center_hex;

  ret.wind = make_center_wind_variant(ir.center_hex, ir.window_xy_span[0],
                                      ir.window_xy_span[1], ir.float_t_prec,
                                      false, err);

  if (!err.empty()) {
    err = fmt::format("Failed to parse center wind. Detail: {}", err);
    return {};
  }

  return ret;
}

fractal_utils::wind_base *
libHybractal::extract_wind_base(center_wind_variant_t &var) noexcept {
  switch (var.index()) {
  case 0:
    return &std::get<0>(var);
  case 1:
    return &std::get<1>(var);
  case 2:
    return &std::get<2>(var);
  case 3:
    return &std::get<3>(var);
  default:
    return nullptr;
  }
}

const fractal_utils::wind_base *
libHybractal::extract_wind_base(const center_wind_variant_t &var) noexcept {
  switch (var.index()) {
  case 0:
    return &std::get<0>(var);
  case 1:
    return &std::get<1>(var);
  case 2:
    return &std::get<2>(var);
  case 3:
    return &std::get<3>(var);
  default:
    return nullptr;
  }
}

libHybractal::hybf_ir_new
libHybractal::hybf_metainfo_new::to_ir() const noexcept {
  hybf_ir_new ir;

  ir.sequence_bin = this->sequence_bin;
  ir.sequence_len = this->sequence_len;
  ir.maxit = this->maxit;
  ir.rows = this->rows;
  ir.cols = this->cols;

  auto base_ptr = extract_wind_base(this->wind);

  ir.window_xy_span[0] = base_ptr->displayed_x_span();
  assert(ir.window_xy_span[0] != 0);
  ir.window_xy_span[1] = base_ptr->displayed_y_span();
  assert(ir.window_xy_span[1] != 0);

  ir.float_t_prec =
      libHybractal::variant_index_to_precision(this->wind.index());

  bool override_old_centerhex = false;
  if (this->generation() == 0) {
    override_old_centerhex = true;
  }

  if (this->chx.empty() || override_old_centerhex) {

    uint8_t buffer[4096];

    auto encoder = [&buffer](auto &wind) {
      using flt_t = std::decay_t<decltype(wind.center[0])>;

      return encode_array2(wind.center, buffer, sizeof(buffer));
    };

    auto bin_bytes = std::visit(encoder, this->wind);
    assert(bin_bytes.has_value());

    std::string hex;
    hex.resize(4096);

    auto char_bytes = fractal_utils::bin_2_hex(buffer, bin_bytes.value(),
                                               hex.data(), hex.size(), true);
    assert(char_bytes.has_value());

    hex.resize(char_bytes.value());

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

  const int8_t version_in_file_header = bfile.header.custom_part()[0];

  hybf_archive result;
  {
    auto blkp_meta = bfile.find_block_single(id_metainfo);
    if (blkp_meta == nullptr) {
      err->assign("metadata not found.");
      return {};
    }

    switch (version_in_file_header) {
    case 0: // The first generation, two possible formats
    {
      result.metainfo() = hybf_metainfo_new::parse_metainfo_gen0(
          blkp_meta->data, blkp_meta->bytes, *err);
      if (!err->empty()) {
        *err =
            fmt::format("Failed to parse metainfo of gen 0. Detail: {}", *err);
        return {};
      }
    } break;

    case 1: // The second generation, ir is employed, and floating points are
      // encoded in ieee
      result.metainfo() = hybf_metainfo_new::parse_metainfo_gen1(
          blkp_meta->data, blkp_meta->bytes, *err);

      if (!err->empty()) {
        *err =
            fmt::format("Failed to parse metainfo of gen 1. Detail: {}", *err);
        return {};
      }
      break;
    default:

      err->assign(fmt::format("Unknown generation number {} in file header.",
                              int(version_in_file_header)));
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
          "Size of mat_age mismatch. Expected {} but in fact {} bytes.",
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
            "Size of mat_age mismatch. Expected {} but in fact {} bytes.",
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

  bfile.header.custom_part()[0] = this->metainfo().generation();

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
