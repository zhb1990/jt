export module jt:log.formatter;

import std;
import :detail.buffer;
import :detail.memory;

export namespace jt::log {

struct message;

struct formatter {
  virtual ~formatter() = default;

  virtual void format(const message& msg, detail::buffer_1k& buf,
                      std::size_t& color_start, std::size_t& color_stop) = 0;
};

using formatter_ptr = detail::dynamic_unique_ptr<formatter>;

}  // namespace jt::log