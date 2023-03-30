#include <fmt/format.h>
#include <hex_convert.h>
#include <libHybractal.h>

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "videotool.h"

std::string hybf_filename(const common_info &ci, int frameidx) noexcept {
  return fmt::format("{}frame{:06}.hybf", ci.hybf_prefix, frameidx);
}

std::string png_filename(const common_info &ci, int frameidx,
                         int pngidx) noexcept {
  return fmt::format("{}frame{:06}-png{:06}.png", ci.png_prefix, frameidx,
                     pngidx);
}

using njson = nlohmann::json;

common_info parse_common(const njson &) noexcept(false);
compute_task parse_compute(const njson &) noexcept(false);
render_task parse_render(const njson &) noexcept(false);
full_task parse_videotask(const njson &) noexcept(false);

std::optional<full_task> load_task(std::string_view filename) noexcept {
  full_task ret;
  try {
    std::ifstream ifs(filename.data());
    njson jo = njson::parse(ifs, nullptr, true, true);
    ret = parse_videotask(jo);
  } catch (std::exception &e) {
    std::cerr << fmt::format("Failed to parse {}, detail: {}", filename,
                             e.what())
              << std::endl;
    return std::nullopt;
  }

  return ret;
}

common_info parse_common(const njson &jo) noexcept(false) {
  common_info ret;
  if (jo.contains("hybf-prefix")) {
    ret.hybf_prefix = jo.at("hybf-prefix");
  } else {
    ret.hybf_prefix = "";
  }

  if (jo.contains("png-prefix")) {
    ret.png_prefix = jo.at("png-prefix");
  } else {
    ret.png_prefix = "";
  }

  ret.rows = jo.at("rows");
  ret.cols = jo.at("cols");
  if (ret.rows <= 0 || ret.cols <= 0) {
    throw std::runtime_error{fmt::format(
        "Rows and cols should be positive number, but rows = {}, and cols = {}",
        ret.rows, ret.cols)};
    return {};
  }

  if ((ret.rows * ret.cols) % 64 != 0) {
    throw std::runtime_error{fmt::format(
        "Num of pixels({}) should be multiples of 64.", ret.rows * ret.cols)};
    return {};
  }

  ret.maxit = jo.at("maxit");

  if (ret.maxit <= 0 || ret.maxit > ::libHybractal::maxit_max) {
    throw std::runtime_error{
        fmt::format("Invalid value for maxit: {}", ret.maxit)};
    return {};
  }

  ret.frame_num = jo.at("frame-num");
  if (ret.frame_num <= 0) {
    throw std::runtime_error{fmt::format(
        "frame-num should be positive, but it is {}", ret.frame_num)};
  }

  ret.ratio = jo.at("ratio");
  if (ret.ratio <= 1) {
    throw std::runtime_error{
        fmt::format("ratio should be greater than 1, but it is {}", ret.ratio)};
  }

  return ret;
}

compute_task parse_compute(const njson &jo) noexcept(false) {
  compute_task ret;

  ret.center_hex = jo.at("centerhex");

  ret.y_span = jo.at("y-span");
  if (ret.y_span <= 0) {
    throw std::runtime_error{"y-span should be positive."};
  }

  if (jo.contains("x-span")) {
    ret.x_span = jo.at("x-span");
  } else {
    ret.x_span = -1;
  }

  ret.threads = jo.at("threads");
  if (ret.threads <= 0) {
    throw std::runtime_error{fmt::format("threads = {}", ret.threads)};
  }

  return ret;
}

render_task parse_render(const njson &jo) noexcept(false) {
  render_task ret;

  ret.config_file = jo.at("config-file");

  ret.threads = jo.at("threads");
  if (ret.threads <= 0) {
    throw std::runtime_error{fmt::format("threads = {}", ret.threads)};
  }

  ret.png_per_frame = jo.at("png-per-frame");
  if (ret.png_per_frame <= 0) {
    throw std::runtime_error{
        fmt::format("png-per-frame = {}", ret.png_per_frame)};
  }

  ret.extra_png_num = jo.at("extra-png-num");
  if (ret.extra_png_num < 0) {
    throw std::runtime_error{
        fmt::format("extra-png-num = {}", ret.extra_png_num)};
  }

  return ret;
}

full_task parse_videotask(const njson &jo) noexcept(false) {
  full_task task;

  try {
    task.common = parse_common(jo.at("common"));
  } catch (std::exception &e) {
    throw std::runtime_error{
        fmt::format("Failed to parse common. Detail: {}", e.what())};
  }

  try {
    task.compute = parse_compute(jo.at("compute"));
  } catch (std::exception &e) {
    throw std::runtime_error{
        fmt::format("Failed to parse compute. Detail: {}", e.what())};
  }

  try {
    task.render = parse_render(jo.at("render"));
  } catch (std::exception &e) {
    throw std::runtime_error{
        fmt::format("Failed to parse render. Detail: {}", e.what())};
  }

  return task;
}