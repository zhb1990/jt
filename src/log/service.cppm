module;

#include "../detail/config.h"

export module jt:log.service;

import std;
import :detail.memory;
import :detail.buffer;
import :log.fwd;
import :log.level;

export namespace jt::log {

class service_impl;

class JT_API service {
 public:
  service();

  ~service() noexcept;

  logger_sptr find(std::string_view name);

  void erase(std::string_view name);

  void clear();

  void start();

  void stop();

  auto get_default() -> logger_sptr;

  void set_default(logger_sptr ptr);

  void flush(logger_wptr ptr);

  void log(logger_wptr ptr, std::uint32_t sid, level lv, detail::buffer_1k& buf,
           const std::source_location& source);
};

}  // namespace jt::log