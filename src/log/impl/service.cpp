module;

#include <lz4frame.h>

// module jt:log.service;
module jt;

import std;
import :log.message;
import :detail.intrusive_mpsc_queue;
import :detail.string;
import :detail.vector;
import :detail.deque;
import :detail.unordered_map;
import :detail.cpu_pause;

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

struct lz4_data {
  lz4_data() {  // NOLINT(*-pro-type-member-init)
    input_chunk.resize(lz4_input_chunk_size);
    output_buff.resize(
        LZ4F_compressBound(lz4_input_chunk_size, &lz4_preferences));
    if (const size_t ec = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
        LZ4F_isError(ec)) {
      throw lz4_exception(ec);  // NOLINT
    }
  }

  ~lz4_data() noexcept { LZ4F_freeCompressionContext(ctx); }

  void compress(  // NOLINT(*-convert-member-functions-to-static)
      const detail::string& src, const detail::string& directory) {
    auto stamp = std::chrono::high_resolution_clock::now();
    // 转成utf-8指针
    std::ifstream input;
    std::u8string_view u8strv(reinterpret_cast<const char8_t*>(src.c_str()),
                              src.size());
    std::filesystem::path path_src = u8strv;
    input.open(path_src, std::ios_base::binary);
    if (!input.is_open()) {
      print_stderr("compress open input {} fail\n", src);
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
      print_stderr("compress open output {} fail\n", src);
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
      auto rate =
          static_cast<double>(count_out) / static_cast<double>(count_in);
      print_stdout("{}: compress {} -> {} bytes, {:.2}, {}ms\n", src, count_in,
                   count_out, rate, cost);
    }

    input.close();
    if (std::error_code ec; !std::filesystem::remove(path_src, ec)) {
      print_stderr("after compress remove fail, {}\n",
                   detail::system_category().message(ec.value()));
    }
  }

  bool compress_file(std::ofstream& output, std::uint64_t& count_out,
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
      print_stderr("Failed to end compression: error 0x{:x}\n",
                   compressed_size);
      return false;
    }
    output.write(output_buff.data(),
                 static_cast<std::streamsize>(compressed_size));
    count_out += compressed_size;

    return true;
  }

  detail::vector<char> input_chunk;
  detail::vector<char> output_buff;
  LZ4F_compressionContext_t ctx{nullptr};
};

class service_impl {
 public:
  using logger_sptr = std::shared_ptr<logger>;
  using logger_wptr = std::weak_ptr<logger>;

  ~service_impl() { stop(); }

  void register_logger(logger_sptr& ptr) {  // NOLINT
    const auto name = ptr->get_name();
    std::scoped_lock lock{loggers_mutex_};
    if (const auto it = loggers_.find(name); it != loggers_.end()) {
      loggers_.erase(it);
    }
    loggers_.emplace(name, ptr);
  }

  logger_sptr find(const std::string_view name) {  // NOLINT
    std::scoped_lock lock{loggers_mutex_};
    if (const auto it = loggers_.find(name); it == loggers_.end()) {
      return {};
    } else {
      return it->second;
    }
  }

  void erase(const std::string_view name) {  // NOLINT
    std::scoped_lock lock{loggers_mutex_};
    loggers_.erase(name);
  }

  void clear() {  // NOLINT(*-convert-member-functions-to-static)
    {
      std::scoped_lock lock{loggers_mutex_};
      loggers_.clear();
    }
    set_default({});
  }

  void start() {  // NOLINT(*-convert-member-functions-to-static)
    if (writer_thread_.joinable() || lz4_thread_.joinable()) return;

    writer_thread_ = std::thread{[this]() { return writer_run(); }};

    lz4_thread_ = std::thread{[this]() { return lz4_run(); }};
  }

  void stop() {
    {
      std::scoped_lock lock{writer_mutex_};
      writer_stop_requested_ = true;
      writer_cv_.notify_one();
    }
    if (writer_thread_.joinable()) {
      writer_thread_.join();
    }

    {
      std::scoped_lock lock{lz4_mutex_};
      lz4_stop_requested_ = true;
      lz4_cv_.notify_one();
    }
    if (lz4_thread_.joinable()) {
      lz4_thread_.join();
    }
  }

  auto get_default() -> logger_sptr {  // NOLINT
#if defined(__clang__)
    std::scoped_lock lock{loggers_mutex_};
    return default_logger_;
#else
    return default_logger_.load(std::memory_order::relaxed);
#endif
  }

  void set_default(const logger_sptr& ptr) {  // NOLINT
#if defined(__clang__)
    std::scoped_lock lock{loggers_mutex_};
    default_logger_ = ptr;
#else
    default_logger_.store(ptr, std::memory_order::relaxed);
#endif
  }

  void flush(const logger_wptr& ptr) {
    message* msg = message_allocator_.allocate(1);
    message_allocator_.construct(msg);
    msg->logger = ptr;
    msg->type = message_type::flush;
    return push_log_message(msg);
  }

  void log(const logger_wptr& ptr, const std::uint32_t sid, const level lv,
           detail::buffer_1k& buf, const std::source_location& source) {
    message* msg = message_allocator_.allocate(1);
    message_allocator_.construct(msg);
    msg->logger = ptr;
    msg->type = message_type::log;
    msg->buf = std::move(buf);
    msg->source = source;
    msg->lv = lv;
    msg->sid = sid;
    msg->point = std::chrono::system_clock::now();
    msg->tid = detail::tid();
    return push_log_message(msg);
  }

  void post_lz4(const std::filesystem::path& file_name,  // NOLINT
                const std::string_view lz4_directory) {
    const auto str = file_name.generic_u8string();
    lz4_message msg;
    msg.tp = lz4_message::type::lz4;
    msg.lz4_directory = lz4_directory;
    msg.file_name.assign(reinterpret_cast<const char*>(str.c_str()),
                         str.size());
    return push_lz4_message(msg);
  }

  void clear_lz4(const detail::string& name,
                 const std::string_view lz4_directory,
                 const std::uint32_t keep_days) {
    lz4_message msg;
    msg.tp = lz4_message::type::clear;
    msg.lz4_directory = lz4_directory;
    msg.file_name = name;
    msg.keep_days = keep_days;
    return push_lz4_message(msg);
  }

 private:
  struct lz4_message;
  void push_lz4_message(lz4_message& msg) {  // NOLINT
    std::scoped_lock lock{lz4_mutex_};
    if (lz4_stop_requested_) return;

    lz4_queue_.emplace_back(std::move(msg));
    lz4_cv_.notify_one();
  }
  void push_log_message(message* msg) {
    std::ptrdiff_t n =
        writer_submission_counter_.fetch_add(1, std::memory_order::relaxed);
    if (n < 0) {
      message_allocator_.destroy(msg);
      message_allocator_.deallocate(msg, 1);
      writer_submission_counter_.compare_exchange_strong(
          n, thread_closed, std::memory_order::relaxed);
      return;
    }

    if (writer_queue_.push_back(msg)) {
      std::scoped_lock lock{writer_mutex_};
      writer_ready_ = true;
      writer_cv_.notify_one();
    }
    writer_submission_counter_.fetch_sub(1, std::memory_order::relaxed);
  }

  inline void writer_do_message() {
    // ReSharper disable once CppDFAUnreachableCode
    // ReSharper disable once CppDFAEndlessLoop
    while (message* msg = writer_queue_.pop_front()) {
      // ReSharper disable once CppDFAEndlessLoop
      if (const auto ptr = msg->logger.lock()) {
        if (msg->type == message_type::log) {
          ptr->backend_log(*msg);
        } else {
          ptr->backend_flush();
        }
      }

      message_allocator_.destroy(msg);
      message_allocator_.deallocate(msg, 1);
    }
  }

  void writer_run() {
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
            expected, thread_closed, std::memory_order::relaxed)) {
          detail::cpu_pause();
          expected = 0;
        }

        writer_do_message();
        break;
      }
    }
  }

  void clear_lz4_files(const lz4_message& msg) {  // NOLINT
    if (msg.keep_days == 0) return;

    namespace fs = std::filesystem;
    using namespace std::chrono;
    using namespace std::chrono_literals;
    const std::u8string_view u8strv{
        reinterpret_cast<const char8_t*>(msg.lz4_directory.c_str()),
        msg.lz4_directory.size()};
    const fs::path directory = u8strv;
    const auto now = system_clock::now();
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

      auto file_time_as_sys =
          std::chrono::clock_cast<std::chrono::system_clock>(last_write);
      if (file_time_as_sys < cutoff) {
        // 删除文件
        if (fs::remove(entry.path(), ec)) {
          print_stdout(
              "Removed old file ({}days old): {}\n",
              duration_cast<hours>(cutoff - file_time_as_sys).count() / 24.0,
              strv);
        } else {
          print_stderr("Failed to remove '{}': {}\n", strv,
                       detail::system_category().message(ec.value()));
        }
      }
    }
  }

  void lz4_run() {  // NOLINT(*-make-member-function-const)
    while (true) {
      detail::deque<lz4_message> queue;
      bool stop_requested = false;
      {
        std::unique_lock lock{lz4_mutex_};
        lz4_cv_.wait_for(lock, std::chrono::seconds(2), [this] {
          return !lz4_queue_.empty() || writer_stop_requested_;
        });
        queue = std::move(lz4_queue_);
        stop_requested = writer_stop_requested_;
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

  struct lz4_message {  // NOLINT(*-pro-type-member-init)
    enum class type { lz4, clear };
    type tp{type::lz4};
    detail::string lz4_directory;
    detail::string file_name;
    std::uint32_t keep_days{0};
  };

  std::mutex loggers_mutex_{};
  detail::unordered_map<std::string_view, logger_sptr> loggers_{};
#if defined(__clang__)
  logger_sptr default_logger_{};
#else
  std::atomic<logger_sptr> default_logger_{};
#endif

  std::thread writer_thread_{};
  std::mutex writer_mutex_{};
  std::condition_variable writer_cv_{};
  detail::intrusive_mpsc_queue<&message::next> writer_queue_{};

  std::thread lz4_thread_{};
  detail::deque<lz4_message> lz4_queue_{};
  std::mutex lz4_mutex_{};
  std::condition_variable_any lz4_cv_{};
  lz4_data lz4_data_;

  bool lz4_stop_requested_{false};
  bool writer_ready_{false};
  bool writer_stop_requested_{false};
  std::atomic<std::ptrdiff_t> writer_submission_counter_{0};
  detail::allocator<message> message_allocator_{};
};

service::service() : impl_(detail::make_unique<service_impl>()) {}  // NOLINT

service::~service() noexcept = default;

// ReSharper disable once CppMemberFunctionMayBeConst
auto service::find(const std::string_view name) -> logger_sptr {
  return impl_->find(name);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void service::erase(const std::string_view name) { return impl_->erase(name); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::clear() { return impl_->clear(); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::start() { return impl_->start(); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::stop() { return impl_->stop(); }

// ReSharper disable once CppMemberFunctionMayBeConst
auto service::get_default() -> logger_sptr { return impl_->get_default(); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::set_default(const logger_sptr& ptr) {
  return impl_->set_default(ptr);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void service::flush(const logger_wptr& ptr) { return impl_->flush(ptr); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::log(const logger_wptr& ptr, const std::uint32_t sid,
                  const level lv, detail::buffer_1k& buf,
                  const std::source_location& source) {
  return impl_->log(std::move(ptr), sid, lv, buf, source);
}

auto service::create_logger(const std::string_view& name,  // NOLINT
                            const bool async, detail::vector<sink_ptr>& sinks)
    -> logger_sptr {
  auto ptr = std::allocate_shared<logger>(detail::allocator<logger>{}, *this,
                                          name, std::move(sinks), async);
  impl_->register_logger(ptr);
  return ptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void service::post_lz4(const std::filesystem::path& file_name,
                       const std::string_view lz4_directory) {
  return impl_->post_lz4(file_name, lz4_directory);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void service::clear_lz4(const detail::string& name,
                        const std::string_view lz4_directory,
                        const std::uint32_t keep_days) {
  return impl_->clear_lz4(name, lz4_directory, keep_days);
}

}  // namespace jt::log