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

// ffmpeg -f image2 -r 60 -i ./png/frame000000-png%06d.png -vf
// "scale=width=1080:height=720" -c:v libx265 -crf 18 h265.mp4

// ffmpeg -f lavfi -i nullsrc=size=1080x720:r=60:duration=10 -i red.mp4 -i
// black.mp4 -filter_complex
// "[0]geq=r='128':g='128':b='128'[alpha];[1][alpha]alphamerge[reda];[2][reda]overlay[out]"
// -map [out] -y temp.mp4

// ffmpeg -r 60 -f image2 -start_number 0 -i ./png/frame000000-png%06d.png
// -frames:v 60 -vf "scale=size=960x540" -y temp0.mp4

// ffmpeg -r 60 -f image2 -start_number 60 -i ./png/frame000000-png%06d.png
// -frames:v 5 -vf "scale=size=960x540" -y temp0e.mp4

#include <fmt/format.h>
#include <omp.h>

#include <atomic>
#include <filesystem>
#include <fstream>

#include "videotool.h"

namespace stdfs = std::filesystem;

std::string ir_video_name(int frame_idx, bool is_extra, const common_info &ci,
                          const video_task &vt) noexcept {
  assert(frame_idx >= 0);
  assert(frame_idx < ci.frame_num);

  return fmt::format("{}ir-frame{:06}-{}{}", ci.video_prefix, frame_idx,
                     (is_extra ? "extra" : "common"),
                     vt.itermediate_config.extension);
}

std::string ir_second_name(int frameidx, const common_info &ci,
                           const video_task &vt) noexcept {
  assert(frameidx >= 0);
  assert(frameidx < ci.frame_num);
  return fmt::format("{}ir-second{:06}{}", ci.video_prefix, frameidx,
                     vt.itermediate_config.extension);
}

std::string encode_expr(const video_task::video_config &cfg) noexcept {
  //-c:v libx265 -crf 18
  return fmt::format("-c:v {} {}", cfg.encoder, cfg.encoder_flags);
}

std::array<int, 2> video_size(const common_info &ci) noexcept {
  return {int(ci.rows / ci.ratio), int(ci.cols / ci.ratio)};
}

std::string size_expression(const common_info &ci) noexcept {
  const auto size = video_size(ci);
  return fmt::format("{}x{}", size[1], size[0]);
}

int run_command(std::string_view command, bool is_dry_run) noexcept {
  if (is_dry_run) {
    std::cout << command << std::endl;
    return 0;
  }

  return system(command.data());
}

bool write_concate_sources(std::string_view filename,
                           const std::vector<std::string> &mp4s) noexcept;

bool run_makevideo(const common_info &ci, const render_task &rt,
                   const video_task &vt, bool dry_run) noexcept {
  // produce ir videos

  const std::string video_size_str = size_expression(ci);
  const std::string ir_encoder_expr = encode_expr(vt.itermediate_config);
  const int fps = rt.png_per_frame;

  std::cout << "Making ir videos" << std::endl;

  std::atomic_int err_counter = 0;
  std::atomic_int task_counter = 0;
  omp_set_num_threads(vt.threads);

#pragma omp parallel for schedule(dynamic)
  for (int fidx = 0; fidx < ci.frame_num; fidx++) {
    std::string out_filename;
    std::string png_filename_expr;
    png_filename_expr = png_filename_expression(ci, fidx);

    std::string command;

    out_filename = ir_video_name(fidx, false, ci, vt);
    if (!stdfs::exists(out_filename)) {
      // ffmpeg -r 60 -f image2 -start_number 0 -i ./png/frame000000-png%06d.png
      // -frames:v 60 -vf "scale=size=960x540" -y temp0.mp4
      command = fmt::format(
          "{} -loglevel quiet -r {} -f image2 -start_number 0 -i {} -frames:v "
          "{} -vf "
          "\"scale={}\" {} -y {}",
          vt.ffmpeg_exe, fps, png_filename_expr, fps, video_size_str,
          ir_encoder_expr, out_filename);

      task_counter++;
      if (run_command(command, dry_run) != 0) {
        err_counter++;
      }
    }

    if (fidx == ci.frame_num - 1) {
      continue;
    }

    if (rt.extra_png_num <= 0) {
      continue;
    }

    out_filename = ir_video_name(fidx, true, ci, vt);
    if (!stdfs::exists(out_filename)) {
      // ffmpeg -r 60 -f image2 -start_number 60 -i
      // ./png/frame000000-png%06d.png -frames:v 5 -vf "scale=size=960x540" -y
      // temp0e.mp4
      command = fmt::format(
          "{} -loglevel quiet -r {} -f image2 -start_number {} -i {} -frames:v "
          "{} -vf "
          "\"scale={}\" {} -y {}",
          vt.ffmpeg_exe, fps, fps, png_filename_expr, rt.extra_png_num,
          video_size_str, ir_encoder_expr, out_filename);

      task_counter++;
      if (run_command(command, dry_run) != 0) {
        err_counter++;
      }
    }
  }

  if (err_counter != 0) {
    std::cerr << fmt::format("{} commands out of {} failed to execute.",
                             err_counter, task_counter);
    return false;
  }

  ////////////////////////////////
  std::cout << "Making ir second videos" << std::endl;

  std::vector<std::string> concat_source_mp4;
  concat_source_mp4.resize(ci.frame_num);

#pragma omp parallel for schedule(dynamic)
  for (int fidx = 0; fidx < ci.frame_num; fidx++) {
    const std::string outname = ir_second_name(fidx, ci, vt);

    concat_source_mp4[fidx] = outname;

    if (stdfs::exists(outname)) {
      continue;
    }

    const std::string src_common = ir_video_name(fidx, false, ci, vt);

    if (fidx == 0) {
#ifdef _WIN32_
      stdfs::copy_options opt{};
#else
      stdfs::copy_options opt{stdfs::copy_options::create_symlinks};
#endif
      opt |= stdfs::copy_options::overwrite_existing;

      std::error_code err;
      stdfs::copy(stdfs::canonical(src_common), outname, opt, err);
      task_counter++;
      if (err) {
        err_counter++;
      }

      continue;
    }

    const std::string src_extra = ir_video_name(fidx - 1, true, ci, vt);

    const std::string rgb_expr =
        fmt::format("'gte({0},N)*({0}.0-N)/{1}.0*255'", rt.extra_png_num,
                    rt.extra_png_num + 1);
    const std::string geq_expr = fmt::format("geq=r={0}:g={0}:b={0}", rgb_expr);

    const std::string i0_expr = fmt::format("-i {}", src_common);
    const std::string i1_expr = fmt::format("-i {}", src_extra);
    const std::string i2_expr = fmt::format(
        "-f lavfi -i nullsrc=size={}:r={}:duration=1", video_size_str, fps);

    const std::string filter_expr = fmt::format(
        "[2]{}[alpha];[1][alpha]alphamerge[extra_a];[0][extra_a]overlay[out]",
        geq_expr);

    const std::string command = fmt::format(
        "{} -loglevel quiet {} {} {} -filter_complex \"{}\" {} -map \"[out]\" "
        "-y {}",
        vt.ffmpeg_exe, i0_expr, i1_expr, i2_expr, filter_expr, ir_encoder_expr,
        outname);

    task_counter++;
    if (run_command(command, dry_run) != 0) {
      err_counter++;
    }
  }

  if (err_counter != 0) {
    std::cerr << fmt::format("{} commands out of {} failed to execute.",
                             err_counter, task_counter);
    return false;
  }
  const std::string concat_sources_filename =
      ci.video_prefix + "concat_sources.txt";

  task_counter++;
  if (!write_concate_sources(concat_sources_filename, concat_source_mp4)) {
    std::cerr << fmt::format("Failed to write concate sources to \"{}\"",
                             concat_sources_filename)
              << std::endl;
    return false;
  }
  ///////////////////////
  std::cout << "Making product video..." << std::endl;
  {
    std::string product_name;
    if (!vt.product_name.empty()) {
      product_name = vt.product_name + vt.product_config.extension;
    } else {
      product_name = ci.video_prefix + "product" + vt.product_config.extension;
    }

    std::string product_encode_expr =
        fmt::format("-c:v {} {}", vt.product_config.encoder,
                    vt.product_config.encoder_flags);

    // ffmpeg -f concat -safe 0 -i ./video/concat_sources.txt -c:v libx265 -crf
    // 18 -y product.mp4
    std::string command =
        fmt::format("{} -f concat -safe 0 -i {} {} -y {}", vt.ffmpeg_exe,
                    concat_sources_filename, product_encode_expr, product_name);

    task_counter++;
    if (run_command(command, dry_run) != 0) {
      err_counter++;
    }
  }

  if (err_counter != 0) {
    std::cerr << fmt::format("{} commands out of {} failed to execute.",
                             err_counter, task_counter);
    return false;
  }

  return true;
}

bool write_concate_sources(std::string_view filename,
                           const std::vector<std::string> &mp4s) noexcept {
  std::ofstream ofs{filename.data()};
  if (!ofs) {
    return false;
  }
  for (const auto &filename : mp4s) {
    ofs << fmt::format("file \'{}\'\n", stdfs::canonical(filename).string());
    if (ofs.bad()) {
      return false;
    }
  }
  ofs.close();

  return true;
}