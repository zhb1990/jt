module;

#include <lz4frame.h>

module jt.log.core;

import std;
import jt.base.containers;
import jt.log.sink.console;
import :archive;

namespace jt::log {

static constexpr LZ4F_preferences_t lz4_preferences = {
    {LZ4F_max256KB, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame,
     0 /* unknown content size */, 0 /* no dictID */, LZ4F_noBlockChecksum},
    0,         /* compression level; 0 == default */
    0,         /* auto flush */
    0,         /* favor decompression speed */
    {0, 0, 0}, /* reserved, must be set to 0 */
};

constexpr size_t lz4_input_chunk_size = 16ull * 1024;

class lz4_exception final : public std::exception {
 public:
  explicit lz4_exception(const size_t ec) : ec_(ec) {}
  [[nodiscard]] const char* what() const noexcept override {
    return LZ4F_getErrorName(ec_);
  }

 private:
  size_t ec_;
};

lz4_data::lz4_data() {  // NOLINT(*-pro-type-member-init)
  input_chunk.resize(lz4_input_chunk_size);
  output_buff.resize(
      LZ4F_compressBound(lz4_input_chunk_size, &lz4_preferences));
  if (const size_t ec = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
      LZ4F_isError(ec)) {
    throw lz4_exception(ec);  // NOLINT
  }
}

lz4_data::~lz4_data() noexcept { LZ4F_freeCompressionContext(ctx); }

void lz4_data::compress(const base::string& src,
                        const base::string& directory) {
  auto stamp = std::chrono::high_resolution_clock::now();
  // 转成utf-8指针
  std::ifstream input;
  std::u8string_view u8strv(reinterpret_cast<const char8_t*>(src.c_str()),
                            src.size());
  std::filesystem::path path_src = u8strv;
  input.open(path_src, std::ios_base::binary);
  if (!input.is_open()) {
    print_stderr("{}: compress open input fail\n", src);
    return;
  }

  if (ctx_invalid) {
    if (ctx) {
      LZ4F_freeCompressionContext(ctx);
      ctx = nullptr;
    }

    const size_t ec = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
    if (LZ4F_isError(ec)) {
      print_stderr("Failed to recreate LZ4 context: {}\n",
                   LZ4F_getErrorName(ec));
      ctx = nullptr;
      return;
    }

    ctx_invalid = false;
  }

  // 转成utf-8指针
  std::ofstream output;
  u8strv = {reinterpret_cast<const char8_t*>(directory.c_str()),
            directory.size()};
  std::filesystem::path path_dest = u8strv;
  path_dest /= path_src.filename();
  path_dest.replace_extension(".log.lz4.tmp");
  output.open(path_dest, std::ios_base::binary | std::ios_base::trunc);
  if (!output.is_open()) {
    print_stderr("{}: compress open output fail\n", src);
    return;
  }

  std::uint64_t count_out = 0;
  std::uint64_t count_in = 0;
  if (!compress_file(output, count_out, count_in, input)) {
    ctx_invalid = true;
    output.close();
    std::error_code ec;
    std::filesystem::remove(path_dest, ec);
    return;
  }

  // Close and verify output before publishing it (also required on Windows).
  output.close();
  if (!output) {
    std::error_code ec;
    std::filesystem::remove(path_dest, ec);
    print_stderr("{}: compress close output fail\n", src);
    return;
  }

  std::error_code ec_rename;
  auto path_final = path_dest;
  path_final
      .replace_extension();  // Remove only .tmp; preserve the source path.
  std::filesystem::rename(path_dest, path_final, ec_rename);
  if (ec_rename) {
    print_stderr("{}: publish archive fail, {}\n", src, ec_rename.message());
    return;
  }

  if (count_in > 0) {
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - stamp)
                    .count();
    auto rate = static_cast<double>(count_out) / static_cast<double>(count_in);
    print_stdout("{}: compress {} -> {} bytes, {:.2f}, {}ms\n", src, count_in,
                 count_out, rate, cost);
  }

  input.close();
  if (std::error_code ec; !std::filesystem::remove(path_src, ec)) {
    print_stderr("{}: after compress remove fail, {}\n", src, ec.message());
  }
}

bool lz4_data::compress_file(std::ofstream& output, std::uint64_t& count_out,
                             std::uint64_t& count_in, std::ifstream& input) {
  /* write frame header */
  auto const header_size = LZ4F_compressBegin(
      ctx, output_buff.data(), output_buff.size(), &lz4_preferences);
  if (LZ4F_isError(header_size)) {
    print_stderr("Failed to start compression: error 0x{:x}\n", header_size);
    return false;
  }

  output.write(output_buff.data(), static_cast<std::streamsize>(header_size));
  count_out = header_size;

  /* stream file */
  while (!input.eof()) {
    input.read(input_chunk.data(), lz4_input_chunk_size);
    const auto read_size = static_cast<size_t>(input.gcount());
    /* nothing left to read from input file */
    if (read_size == 0) break;
    count_in += read_size;

    auto const compressed_size =
        LZ4F_compressUpdate(ctx, output_buff.data(), output_buff.size(),
                            input_chunk.data(), read_size, nullptr);
    if (LZ4F_isError(compressed_size)) {
      print_stderr("Compression failed: error 0x{:x}\n", compressed_size);
      return false;
    }

    output.write(output_buff.data(),
                 static_cast<std::streamsize>(compressed_size));
    count_out += compressed_size;
  }

  /* flush whatever remains within internal buffers */
  auto const compressed_size =
      LZ4F_compressEnd(ctx, output_buff.data(), output_buff.size(), nullptr);
  if (LZ4F_isError(compressed_size)) {
    print_stderr("Failed to end compression: error 0x{:x}\n", compressed_size);
    return false;
  }

  output.write(output_buff.data(),
               static_cast<std::streamsize>(compressed_size));
  count_out += compressed_size;

  return !input.bad() && output.good();
}

archive_worker::~archive_worker() noexcept {
  request_stop();
  wait_stop();
}

void archive_worker::start() {
  lz4_thread_ = std::thread{[this] { lz4_run(); }};
}

void archive_worker::request_stop() {
  std::scoped_lock lock{lz4_mutex_};
  lz4_stop_requested_ = true;
  lz4_cv_.notify_one();
}

void archive_worker::wait_stop() {
  if (lz4_thread_.joinable()) lz4_thread_.join();
}

void archive_worker::post_lz4(const std::filesystem::path& file_name,  // NOLINT
                              const std::string_view lz4_directory) {
  const auto str = file_name.generic_u8string();
  lz4_message msg;
  msg.tp = lz4_message::type::lz4;
  msg.lz4_directory = lz4_directory;
  msg.file_name.assign(reinterpret_cast<const char*>(str.c_str()), str.size());
  return push_lz4_message(msg);
}

void archive_worker::clear_lz4(const base::string& name,
                               const std::string_view lz4_directory,
                               const std::uint32_t keep_days) {
  lz4_message msg;
  msg.tp = lz4_message::type::clear;
  msg.lz4_directory = lz4_directory;
  msg.file_name = name;
  msg.keep_days = keep_days;
  return push_lz4_message(msg);
}

void archive_worker::push_lz4_message(lz4_message& msg) {  // NOLINT
  std::scoped_lock lock{lz4_mutex_};
  if (lz4_stop_requested_) return;

  lz4_queue_.emplace_back(std::move(msg));
  lz4_cv_.notify_one();
}

void archive_worker::clear_lz4_files(const lz4_message& msg) {  // NOLINT
  if (msg.keep_days == 0) return;

  namespace fs = std::filesystem;
  using namespace std::chrono;
  using namespace std::chrono_literals;
  const std::u8string_view u8strv{
      reinterpret_cast<const char8_t*>(msg.lz4_directory.c_str()),
      msg.lz4_directory.size()};
  const fs::path directory = u8strv;
  const auto now = file_clock::now();
  const auto cutoff = now - msg.keep_days * 24h;
  std::error_code ec;
  if (!fs::exists(directory, ec)) {
    if (ec) {
      return print_stderr("Error checking existence of '{}': {}\n",
                          msg.lz4_directory, ec.message());
    }
    return print_stderr("Path is not exists '{}'\n", msg.lz4_directory);
  }

  if (!fs::is_directory(directory, ec)) {
    if (ec) {
      return print_stderr("Error checking if directory '{}': {}\n",
                          msg.lz4_directory, ec.message());
    }
    return print_stderr("Path is not a directory: '{}'\n", msg.lz4_directory);
  }

  fs::directory_iterator iter(directory, ec);
  if (ec) {
    return print_stderr("Failed to open directory '{}': {}\n",
                        msg.lz4_directory, ec.message());
  }

  base::string filename_start = msg.file_name + "_";
  const fs::directory_iterator dir_end{};
  for (; iter != dir_end; iter.increment(ec)) {
    if (ec) {
      print_stderr("Failed to iterate directory '{}': {}\n", msg.lz4_directory,
                   ec.message());
      break;
    }

    const auto& entry = *iter;
    std::u8string filename;
    try {
      filename = entry.path().filename().u8string();
    } catch (const fs::filesystem_error& e) {
      print_stderr("cannot convert filename: {}\n", e.what());
      continue;
    }

    const std::string_view strv{reinterpret_cast<const char*>(filename.c_str()),
                                filename.size()};
    if (!entry.is_regular_file(ec)) {
      if (ec) {
        print_stderr("cannot stat file '{}': {}\n", strv, ec.message());
      }
      continue;
    }

    if (!strv.starts_with(filename_start) || !strv.ends_with(".log.lz4")) {
      continue;
    }

    // 获取最后修改时间
    auto last_write = entry.last_write_time(ec);
    if (ec) {
      print_stderr("cannot get mtime of '{}': {}\n", strv, ec.message());
      continue;
    }

    if (last_write < cutoff) {
      // 删除文件
      if (fs::remove(entry.path(), ec)) {
        print_stdout("Removed old file ({}days old): {}\n",
                     duration_cast<hours>(cutoff - last_write).count() / 24.0,
                     strv);
      } else {
        print_stderr("Failed to remove '{}': {}\n", strv, ec.message());
      }
    }
  }
}

void archive_worker::lz4_run() {  // NOLINT(*-make-member-function-const)
  while (true) {
    base::deque<lz4_message> queue;
    bool stop_requested = false;
    try {
      std::unique_lock lock{lz4_mutex_};
      lz4_cv_.wait_for(lock, std::chrono::seconds(2), [this] {
        return !lz4_queue_.empty() || lz4_stop_requested_;
      });
      queue = std::move(lz4_queue_);
      stop_requested = lz4_stop_requested_;
    } catch (const std::exception& e) {
      print_stderr("lz4 worker exception: {}\n", e.what());
      continue;
    } catch (...) {
      print_stderr("lz4 worker unknown exception\n");
      continue;
    }

    for (auto& msg : queue) {
      try {
        if (msg.tp == lz4_message::type::lz4) {
          lz4_data_.compress(msg.file_name, msg.lz4_directory);
        } else if (msg.tp == lz4_message::type::clear) {
          clear_lz4_files(msg);
        }
      } catch (const std::exception& e) {
        print_stderr("lz4 worker exception: {}\n", e.what());
        // 异常可能发生在 compressBegin 之后：这里再 reset_ctx()
      } catch (...) {
        print_stderr("lz4 worker unknown exception\n");
      }
    }

    if (stop_requested) break;
  }
}

}  // namespace jt::log
