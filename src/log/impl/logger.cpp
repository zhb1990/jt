// module jt:log.logger;
module jt;

import std;
import :detail.string;
import :detail.vector;
import :log.message;

namespace jt::log {

class logger_impl {
 public:
  logger_impl(service& service, const std::string_view& name,  // NOLINT
              detail::vector<service::sink_ptr>& sinks, const bool async)
      : service_(service),
        name_(name),
        sinks_(std::move(sinks)),
        async_(async) {}

  void set_level(const level lv) noexcept {
    lv_.store(lv, std::memory_order::relaxed);
  }

  [[nodiscard]] auto get_level() const noexcept -> level {
    return lv_.load(std::memory_order::relaxed);
  }

  [[nodiscard]] auto get_name() const noexcept -> std::string_view {
    return name_;
  }

  [[nodiscard]] auto is_async() const noexcept -> bool { return async_; }

  [[nodiscard]] auto get_service() const noexcept -> service& {
    return service_;
  }

  void backend_log(const message& msg) const {
    for (const auto& sink : sinks_) {
      try {
        sink->log(msg);
      } catch (...) {
      }
    }
  }

  void backend_flush() const {
    for (const auto& sink : sinks_) {
      try {
        sink->flush();
      } catch (...) {
      }
    }
  }

 private:
  service& service_;
  detail::string name_;
  detail::vector<service::sink_ptr> sinks_;
  std::atomic<level> lv_{level::trace};
  bool async_;
};

logger::logger(service& service, const std::string_view& name,  // NOLINT
               detail::vector<sink_ptr> sinks, bool async)
    : impl_(detail::make_unique<logger_impl>(service, name, sinks, async)) {}
logger::~logger() noexcept = default;

// ReSharper disable once CppMemberFunctionMayBeConst
void logger::set_level(const level lv) noexcept { return impl_->set_level(lv); }

auto logger::get_level() const noexcept -> level { return impl_->get_level(); }

auto logger::get_name() const noexcept -> std::string_view {
  return impl_->get_name();
}

void logger::flush() {  // NOLINT(*-convert-member-functions-to-static)
  if (!impl_->is_async()) {
    return impl_->backend_flush();
  }

  return impl_->get_service().flush(weak_from_this());
}

auto logger::should_log(level lv) const noexcept -> bool {
  return static_cast<std::uint8_t>(lv) <=
         static_cast<std::uint8_t>(impl_->get_level());
}

void logger::log(std::uint32_t sid, level lv, detail::buffer_1k& buf,  // NOLINT
                 const std::source_location& source) {
  auto& service = impl_->get_service();
  if (!impl_->is_async()) {
    message msg;
    msg.buf = std::move(buf);
    msg.source = source;
    msg.lv = lv;
    msg.sid = sid;
    msg.point = std ::chrono::system_clock::now();
    msg.type = message_type::log;
    msg.tid = detail::tid();
    return impl_->backend_log(msg);
  }

  return service.log(weak_from_this(), sid, lv, buf, source);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void logger::backend_log(const message& msg) { return impl_->backend_log(msg); }

// ReSharper disable once CppMemberFunctionMayBeConst
void logger::backend_flush() { return impl_->backend_flush(); }

}  // namespace jt::log