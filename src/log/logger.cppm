module;

#include "../detail/config.h"

export module jt:log.logger;

import std;
import :detail.memory;
import :detail.buffer;
import :detail.vector;
import :log.sink;
import :log.fwd;

export namespace jt::log {

class logger_impl;

class logger : public std::enable_shared_from_this<logger> {
 public:
  friend class service_impl;
  using sink_ptr = detail::dynamic_unique_ptr<sink>;
  logger(service& service, const std::string_view& name,
         detail::vector<sink_ptr> sinks, bool async);

  JT_API ~logger() noexcept;

  JT_API void set_level(level lv) noexcept;

  [[nodiscard]] JT_API auto get_level() const noexcept -> level;

  [[nodiscard]] JT_API auto get_name() const noexcept -> std::string_view;

  JT_API void flush();

  [[nodiscard]] JT_API auto should_log(level lv) const noexcept -> bool;

  JT_API void log(std::uint32_t sid, level lv, detail::buffer_1k& buf,
                  const std::source_location& source);

 protected:
  void backend_log(const message& msg);

  void backend_flush();

 private:
  detail::unique_ptr<logger_impl> impl_;
};

}  // namespace jt::log