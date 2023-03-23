#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "libRender.h"
using std::cout, std::endl;

using njson = nlohmann::json;

libHybractal::hsv_render_option::hsv_range parse_range(
    const njson &jo) noexcept(false);

std::optional<libHybractal::hsv_render_option> parse_option(
    const njson &jo) noexcept;

std::optional<libHybractal::hsv_render_option>
libHybractal::hsv_render_option::load(const char *beg,
                                      const char *end) noexcept {
  njson jo;

  try {
    jo = njson::parse(beg, end, nullptr, true, true);
  } catch (std::exception &err) {
    cout << "Failed to parse json. Detail: " << err.what() << endl;
    return std::nullopt;
  }

  return parse_option(jo);
}

std::optional<libHybractal::hsv_render_option>
libHybractal::hsv_render_option::load_from_file(
    std::string_view filename) noexcept {
  njson jo;
  try {
    std::ifstream ifs(filename.data());
    jo = njson::parse(ifs, nullptr, true, true);
  } catch (std::exception &err) {
    cout << "Failed to parse json. Detail: " << err.what() << endl;
    return std::nullopt;
  }
  return parse_option(jo);
}

std::array<float, 2> parse_single_range(const njson &range_val) noexcept(
    false) {
  if (range_val.is_array()) {
    return {range_val[0], range_val[1]};
  }

  return {range_val, range_val};
}

libHybractal::frac_val string_to_frac_val(std::string_view sv) noexcept(false) {
  if (sv == "age") {
    return libHybractal::fv_age;
  }
  if (sv == "norm2") {
    return libHybractal::fv_norm2;
  }
  if (sv == "angle") {
    return libHybractal::fv_angle;
  }
  throw std::runtime_error(fmt::format("Invalid frac_val : {}", sv));
  return {};
}

libHybractal::hsv_render_option::hsv_range parse_range(
    const njson &jo) noexcept(false) {
  libHybractal::hsv_render_option::hsv_range range;

  range.range_H = parse_single_range(jo.at("range_H"));
  range.range_S = parse_single_range(jo.at("range_S"));
  range.range_V = parse_single_range(jo.at("range_V"));

  range.age_peroid = jo.at("age_preoid");

  for (size_t i = 0; i < 3; i++) {
    const std::string &str = jo.at("fv_mapping")[i];

    range.fv_mapping[i] = string_to_frac_val(str);
  }

  return range;
}

std::optional<libHybractal::hsv_render_option> parse_option(
    const njson &jo) noexcept {
  libHybractal::hsv_render_option ret;
  try {
    ret.range_age_inf = parse_range(jo.at("range_age_inf"));
    ret.range_age_normal = parse_range(jo.at("range_age_normal"));
  } catch (std::exception &e) {
    cout << fmt::format("Failed to parse hsv render option. Detail: {}\n",
                        e.what());
    return std::nullopt;
  }

  return ret;
}