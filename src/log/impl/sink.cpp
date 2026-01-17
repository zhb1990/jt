// module jt:log.sink;
module jt;

import std;
import :detail.cache_line;
import :log.message;
import :log.default_formatter;

namespace jt::log {

class sink_impl {
 public:
  sink_impl() {  // NOLINT
    formatter_ = detail::make_dynamic_unique<formatter, default_formatter>();
  }

  void set_level(const level lv) { lv_.store(lv, std::memory_order::relaxed); }

  void log(const message& msg, sink* s) {  // NOLINT
    if (static_cast<std::uint8_t>(msg.lv) >
        static_cast<std::uint8_t>(lv_.load(std::memory_order::relaxed))) {
      return;
    }

    detail::buffer_1k buf;
    std::size_t color_start, color_stop;
    std::lock_guard lock(mtx_);
    formatter_->format(msg, buf, color_start, color_stop);
    return s->write(msg.lv, msg.point, buf, color_start, color_stop);
  }

  void flush(sink* s) {  // NOLINT(*-convert-member-functions-to-static)
    std::lock_guard lock(mtx_);
    return s->flush_unlock();
  }

  void set_formatter(sink::formatter_ptr ptr) {
    std::lock_guard lock(mtx_);
    formatter_ = std::move(ptr);
  }

 private:
  std::atomic<level> lv_{level::trace};
  char padding[detail::cache_line_bytes - sizeof(std::atomic<level>)];

  sink::formatter_ptr formatter_;
  std::mutex mtx_;
};

sink::sink() { impl_ = detail::make_unique<sink_impl>(); }  // NOLINT

sink::~sink() noexcept = default;

// ReSharper disable once CppMemberFunctionMayBeConst
void sink::set_level(const level lv) { return impl_->set_level(lv); }

void sink::log(const message& msg) { return impl_->log(msg, this); }

void sink::flush() { return impl_->flush(this); }

// ReSharper disable once CppMemberFunctionMayBeConst
void sink::set_formatter(formatter_ptr ptr) {
  return impl_->set_formatter(std::move(ptr));
}

}  // namespace jt::log