// module jt:log.service;
module jt;

import std;
import :log.fwd;
import :log.message;
import :detail.intrusive_mpsc_queue;
import :detail.string;

namespace jt::log {

class service_impl {
 public:
  void register_logger(logger_sptr& ptr) {  // NOLINT
    detail::string name;
    name = ptr->get_name();
    {
      std::scoped_lock lock{loggers_mutex_};
      loggers_[name] = ptr;
    }
  }

  void request_stop() {
    {
      std::scoped_lock lock{writer_mutex_};
      writer_stop_requested_ = true;
      writer_cv_.notify_one();
    }
    {
      std::scoped_lock lock{lz4_mutex_};
      lz4_stop_requested_ = true;
      lz4_cv_.notify_one();
    }
  }

  logger_sptr find(const std::string_view name) {  // NOLINT
    detail::string name_str;
    name_str = name;
    {
      std::scoped_lock lock{loggers_mutex_};
      if (const auto it = loggers_.find(name_str); it == loggers_.end()) {
        return {};
      } else {
        return it->second;
      }
    }
  }

  void erase(const std::string_view name) {  // NOLINT
    detail::string name_str;
    name_str = name;
    {
      std::scoped_lock lock{loggers_mutex_};
      loggers_.erase(name_str);
    }
  }

  void clear() {  // NOLINT(*-convert-member-functions-to-static)
    set_default({});
    {
      std::scoped_lock lock{loggers_mutex_};
      loggers_.clear();
    }
  }

  void start() {}

  void stop() {}

  // ReSharper disable once CppMemberFunctionMayBeConst
  auto get_default() -> logger_sptr {
    return default_logger_.load(std::memory_order::relaxed);
  }

  void set_default(logger_sptr ptr) {  // NOLINT
    const auto old = default_logger_.load(std::memory_order::relaxed);
    erase(old->get_name());
    return default_logger_.store(std::move(ptr), std::memory_order::relaxed);
  }

  void flush(logger_wptr ptr) {}

  void log(logger_wptr ptr, std::uint32_t sid, level lv, detail::buffer_1k& buf,
           const std::source_location& source) {}

 private:
  std::mutex loggers_mutex_{};
  std::unordered_map<detail::string, logger_sptr> loggers_{};
  std::atomic<logger_sptr> default_logger_{};

  std::thread writer_thread_{};
  std::mutex writer_mutex_{};
  std::condition_variable writer_cv_{};
  detail::intrusive_mpsc_queue<&message::next> writer_queue_{};

  std::thread lz4_thread_{};
  std::deque<std::string> lz4_queue_{};
  std::mutex lz4_mutex_{};
  std::condition_variable_any lz4_cv_{};

  bool lz4_stop_requested_{false};
  bool writer_ready_{false};
  bool writer_stop_requested_{false};
  std::size_t writer_submission_counter_{1};
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
void service::set_default(logger_sptr ptr) {
  return impl_->set_default(std::move(ptr));
}

// ReSharper disable once CppMemberFunctionMayBeConst
void service::flush(logger_wptr ptr) { return impl_->flush(std::move(ptr)); }

// ReSharper disable once CppMemberFunctionMayBeConst
void service::log(logger_wptr ptr, const std::uint32_t sid, const level lv,
                  detail::buffer_1k& buf, const std::source_location& source) {
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