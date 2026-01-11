export module jt:log.default_formatter;

import std;
import :log.formatter;
import :log.message;

export namespace jt::log {

class default_formatter : public formatter {
 public:
  void format(const message& msg, detail::buffer_1k& buf,
              std::size_t& color_start, std::size_t& color_stop) override {}
};

}  // namespace jt::log