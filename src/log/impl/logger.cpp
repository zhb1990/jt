// module jt:log.logger;
module jt;

import std;
import :detail.string;
import :detail.vector;

namespace jt::log {

class logger_impl {
 public:
  logger_impl(service& service, const std::string_view& name,
              detail::vector<sink_ptr>& sinks, bool async)
      : service_(service),
        name_(name),
        sinks_(std::move(sinks)),
        async_(async) {}

  void set_level(level lv) noexcept {
    lv_.store(lv, std::memory_order::relaxed);
  }

  auto get_level() const noexcept -> level {
    return lv_.load(std::memory_order::relaxed);
  }

  auto get_name() const noexcept -> std::string_view { return name_; }

  void flush() const {}

  auto should_log(level lv) const noexcept -> bool {
    return static_cast<std::uint8_t>(lv) <=
           static_cast<std::uint8_t>(get_level());
  }

  void log(level lv, detail::buffer_1k& buf,
           const std::source_location& source) const {}

  void backend_log(const message& msg) const {}

  void backend_flush() const {}

 private:
  service& service_;
  detail::string name_;
  detail::vector<sink_ptr> sinks_;
  std::atomic<level> lv_{level::trace};
  bool async_;
};

logger::logger(service& service, const std::string_view& name,
               detail::vector<sink_ptr> sinks, bool async)
    : impl_(detail::make_unique<logger_impl>(service, name, sinks, async)) {}
logger::~logger() noexcept = default;

void logger::set_level(level lv) noexcept { return impl_->set_level(lv); }

auto logger::get_level() const noexcept -> level { return impl_->get_level(); }

auto logger::get_name() const noexcept -> std::string_view {
  return impl_->get_name();
}

void logger::flush() const { return impl_->flush(); }

auto logger::should_log(level lv) const noexcept -> bool {
  return impl_->should_log(lv);
}

void logger::log(level lv, detail::buffer_1k& buf,
                 const std::source_location& source) const {
  return impl_->log(lv, buf, source);
}

void logger::backend_log(const message& msg) const {
  return impl_->backend_log(msg);
}

void logger::backend_flush() const { return impl_->backend_flush(); }

}  // namespace jt::log