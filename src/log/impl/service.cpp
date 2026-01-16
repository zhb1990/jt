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
    return push_message(msg);
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
    return push_message(msg);
  }

 private:
  void push_message(message* msg) {
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
  detail::deque<detail::string> lz4_queue_{};
  std::mutex lz4_mutex_{};
  std::condition_variable_any lz4_cv_{};

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

}  // namespace jt::log