module;

#include <lz4frame.h>

module jt;

import std;

import :detail.os;
import :detail.string;
import :detail.unordered_map;
import :detail.cpu_pause;
import :log.message;
import :log.service_impl;

namespace jt::log {

constexpr std::ptrdiff_t thread_closed =
    std::numeric_limits<std::ptrdiff_t>::min() / 2;

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

void lz4_data::compress(  // NOLINT(*-convert-member-functions-to-static)
    const detail::string& src, const detail::string& directory) {
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

  // 转成utf-8指针
  std::ofstream output;
  u8strv = {reinterpret_cast<const char8_t*>(directory.c_str()),
            directory.size()};
  std::filesystem::path path_dest = u8strv;
  path_dest /= path_src.filename();
  path_dest.replace_extension(".log.lz4");
  output.open(path_dest, std::ios_base::binary | std::ios_base::trunc);
  if (!output.is_open()) {
    print_stderr("{}: compress open output fail\n", src);
    return;
  }

  std::uint64_t count_out = 0;
  std::uint64_t count_in = 0;
  if (!compress_file(output, count_out, count_in, input)) {
    return;
  }

  if (count_in > 0) {
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - stamp)
                    .count();
    auto rate = static_cast<double>(count_out) / static_cast<double>(count_in);
    print_stdout("{}: compress {} -> {} bytes, {:.2}, {}ms\n", src, count_in,
                 count_out, rate, cost);
  }

  input.close();
  if (std::error_code ec; !std::filesystem::remove(path_src, ec)) {
    print_stderr("{}: after compress remove fail, {}\n", src,
                 detail::system_category().message(ec.value()));
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

  return true;
}

service_impl::service_impl() {
  writer_thread_ = std::thread{[this]() { return writer_run(); }};
  try {
    lz4_thread_ = std::thread{[this]() { return lz4_run(); }};
  } catch (...) {
    request_stop();
    if (writer_thread_.joinable()) {
      writer_thread_.join();
    }
    throw;
  }
}

service_impl::~service_impl() {
  request_stop();
  if (writer_thread_.joinable()) {
    writer_thread_.join();
  }
  if (lz4_thread_.joinable()) {
    lz4_thread_.join();
  }
}

void service_impl::register_logger(logger_sptr& ptr) {  // NOLINT
  const auto name = ptr->get_name();
  std::scoped_lock lock{loggers_mutex_};
  if (const auto it = loggers_.find(name); it != loggers_.end()) {
    loggers_.erase(it);
  }
  loggers_.emplace(name, ptr);
}

service_impl::logger_sptr service_impl::find(
    const std::string_view name) {  // NOLINT
  std::scoped_lock lock{loggers_mutex_};
  if (const auto it = loggers_.find(name); it == loggers_.end()) {
    return {};
  } else {
    return it->second;
  }
}

void service_impl::erase(const std::string_view name) {  // NOLINT
  std::scoped_lock lock{loggers_mutex_};
  loggers_.erase(name);
}

void service_impl::clear() {  // NOLINT(*-convert-member-functions-to-static)
  {
    std::scoped_lock lock{loggers_mutex_};
    loggers_.clear();
  }
  set_default({});
}

void service_impl::request_stop() {
  std::scoped_lock lock{writer_mutex_};
  writer_stop_requested_.store(true, std::memory_order::relaxed);
  writer_cv_.notify_one();
}

auto service_impl::get_default() -> logger_sptr {  // NOLINT
  return default_logger_.load(std::memory_order::acquire);
}

void service_impl::set_default(const logger_sptr& ptr) {  // NOLINT
  default_logger_.store(ptr, std::memory_order::release);
}

void service_impl::flush(const logger_wptr& ptr) {
  message* msg = new_log_message();
  if (!msg) {
    return;
  }

  msg->target = ptr;
  msg->type = message_type::flush;
  return push_log_message(msg);
}

void service_impl::log(const logger_wptr& ptr, const std::uint32_t sid,
                       const level lv, detail::buffer_1k& buf,
                       const std::source_location& source) {
  message* msg = new_log_message();
  if (!msg) {
    return;
  }

  msg->target = ptr;
  msg->type = message_type::log;
  msg->payload = std::move(buf);
  msg->source = source;
  msg->lv = lv;
  msg->service_id = sid;
  msg->timestamp = std::chrono::system_clock::now();
  msg->thread_id = detail::tid();
  return push_log_message(msg);
}

void service_impl::post_lz4(const std::filesystem::path& file_name,  // NOLINT
                            const std::string_view lz4_directory) {
  const auto str = file_name.generic_u8string();
  lz4_message msg;
  msg.tp = lz4_message::type::lz4;
  msg.lz4_directory = lz4_directory;
  msg.file_name.assign(reinterpret_cast<const char*>(str.c_str()), str.size());
  return push_lz4_message(msg);
}

void service_impl::clear_lz4(const detail::string& name,
                             const std::string_view lz4_directory,
                             const std::uint32_t keep_days) {
  lz4_message msg;
  msg.tp = lz4_message::type::clear;
  msg.lz4_directory = lz4_directory;
  msg.file_name = name;
  msg.keep_days = keep_days;
  return push_lz4_message(msg);
}

void service_impl::push_lz4_message(lz4_message& msg) {  // NOLINT
  std::scoped_lock lock{lz4_mutex_};
  if (lz4_stop_requested_) return;

  lz4_queue_.emplace_back(std::move(msg));
  lz4_cv_.notify_one();
}

message* service_impl::new_log_message() {
  if (writer_stop_requested_.load(std::memory_order_relaxed)) {
    return nullptr;
  }

  message* msg = message_allocator_.allocate(1);
  message_allocator_.construct(msg);
  return msg;
}

void service_impl::delete_log_message(message* msg) {
  message_allocator_.destroy(msg);
  message_allocator_.deallocate(msg, 1);
}

void service_impl::push_log_message(message* msg) {
  std::ptrdiff_t n =
      writer_submission_counter_.fetch_add(1, std::memory_order::relaxed);
  if (n < 0) {
    delete_log_message(msg);
    writer_submission_counter_.fetch_sub(1, std::memory_order::relaxed);
    return;
  }

  if (writer_queue_.push_back(msg)) {
    std::scoped_lock lock{writer_mutex_};
    writer_ready_ = true;
    writer_cv_.notify_one();
  }
  writer_submission_counter_.fetch_sub(1, std::memory_order::release);
}

inline void service_impl::writer_do_message() {
  message* msg = nullptr;
  bool is_empty = false;
  std::tie(msg, is_empty) = writer_queue_.pop_front();
  while (!is_empty) {
    // ReSharper disable once CppDFAEndlessLoop
    if (msg) {
      if (const auto ptr = msg->target.lock()) {
        if (msg->type == message_type::log) {
          ptr->backend_log(msg->record());
        } else {
          ptr->backend_flush();
        }
      }

      delete_log_message(msg);
    } else {
      detail::cpu_pause();
    }

    std::tie(msg, is_empty) = writer_queue_.pop_front();
  }
}

void service_impl::writer_run() {
  while (true) {
    writer_do_message();

    std::unique_lock lock{writer_mutex_};
    writer_cv_.wait_for(lock, std::chrono::seconds(2), [this] {
      return writer_ready_ || writer_stop_requested_;
    });
    const bool stop_requested = writer_stop_requested_;
    writer_ready_ = false;
    lock.unlock();

    if (stop_requested) {
      std::ptrdiff_t expected = 0;
      while (!writer_submission_counter_.compare_exchange_weak(
          expected, thread_closed, std::memory_order::acquire,
          std::memory_order::relaxed)) {
        detail::cpu_pause();
        expected = 0;
      }

      writer_do_message();

      {
        std::scoped_lock lock{lz4_mutex_};
        lz4_stop_requested_ = true;
        lz4_cv_.notify_one();
      }
      break;
    }
  }
}

void service_impl::clear_lz4_files(const lz4_message& msg) {  // NOLINT
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
                          msg.lz4_directory,
                          detail::system_category().message(ec.value()));
    }
    return print_stderr("Path is not exists '{}'\n", msg.lz4_directory);
  }

  if (!fs::is_directory(directory, ec)) {
    if (ec) {
      return print_stderr("Error checking if directory '{}': {}\n",
                          msg.lz4_directory,
                          detail::system_category().message(ec.value()));
    }
    return print_stderr("Path is not a directory: '{}'\n", msg.lz4_directory);
  }

  fs::directory_iterator iter(directory, ec);
  if (ec) {
    return print_stderr("Failed to open directory '{}': {}\n",
                        msg.lz4_directory,
                        detail::system_category().message(ec.value()));
  }

  for (const auto& entry : iter) {
    const auto filename = entry.path().filename().u8string();
    std::string_view strv{reinterpret_cast<const char*>(filename.c_str()),
                          filename.size()};
    if (!entry.is_regular_file(ec)) {
      if (ec) {
        print_stderr("cannot stat file '{}': {}\n", strv,
                     detail::system_category().message(ec.value()));
      }
      continue;
    }

    if (!strv.starts_with(msg.file_name) || !strv.ends_with(".log.lz4")) {
      continue;
    }

    // 获取最后修改时间
    auto last_write = entry.last_write_time(ec);
    if (ec) {
      print_stderr("cannot get mtime of '{}': {}\n", strv,
                   detail::system_category().message(ec.value()));
      continue;
    }

    if (last_write < cutoff) {
      // 删除文件
      if (fs::remove(entry.path(), ec)) {
        print_stdout("Removed old file ({}days old): {}\n",
                     duration_cast<hours>(cutoff - last_write).count() / 24.0,
                     strv);
      } else {
        print_stderr("Failed to remove '{}': {}\n", strv,
                     detail::system_category().message(ec.value()));
      }
    }
  }
}

void service_impl::lz4_run() {  // NOLINT(*-make-member-function-const)
  while (true) {
    detail::deque<lz4_message> queue;
    bool stop_requested = false;
    {
      std::unique_lock lock{lz4_mutex_};
      lz4_cv_.wait_for(lock, std::chrono::seconds(2), [this] {
        return !lz4_queue_.empty() || lz4_stop_requested_;
      });
      queue = std::move(lz4_queue_);
      stop_requested = lz4_stop_requested_;
    }

    for (auto& msg : queue) {
      if (msg.tp == lz4_message::type::lz4) {
        lz4_data_.compress(msg.file_name, msg.lz4_directory);
      } else if (msg.tp == lz4_message::type::clear) {
        clear_lz4_files(msg);
      }
    }

    if (stop_requested) break;
  }
}

}  // namespace jt::log