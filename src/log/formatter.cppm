export module jt:log.formatter;

import std;
import :detail.buffer;
import :log.fwd;

export namespace jt::log {

struct formatter {
  virtual ~formatter() = default;

  virtual void format(const message& msg, detail::buffer_1k& buf,
                      std::size_t& color_start, std::size_t& color_stop) = 0;
};

}  // namespace jt::log