module jt.log.core;

import std;
import :service_impl;

namespace jt::log {

service_impl::service_impl() {
  writer_.start();
  try {
    archive_.start();
  } catch (...) {
    writer_.request_stop();
    writer_.wait_stop();
    throw;
  }
}

service_impl::~service_impl() noexcept {
  request_stop();
  wait_stop();
}

void service_impl::request_stop() { writer_.request_stop(); }

void service_impl::wait_stop() {
  writer_.wait_stop();
  archive_.request_stop();
  archive_.wait_stop();
}

void service_impl::register_logger(logger_sptr& ptr) {  // NOLINT
  const auto name = ptr->get_name();
  std::scoped_lock lock{loggers_mutex_};
  if (const auto it = loggers_.find(name); it != loggers_.end()) {
    loggers_.erase(it);
  }
  loggers_.emplace(name, ptr);
}

service_impl::logger_sptr service_impl::find(
    const std::string_view name) {  // NOLINT
  std::scoped_lock lock{loggers_mutex_};
  if (const auto it = loggers_.find(name); it == loggers_.end()) {
    return {};
  } else {
    return it->second;
  }
}

void service_impl::erase(const std::string_view name) {  // NOLINT
  std::scoped_lock lock{loggers_mutex_};
  loggers_.erase(name);
}

void service_impl::clear() {  // NOLINT(*-convert-member-functions-to-static)
  {
    std::scoped_lock lock{loggers_mutex_};
    loggers_.clear();
  }
  set_default({});
}

auto service_impl::get_default() -> logger_sptr {  // NOLINT
  return default_logger_.load(std::memory_order::acquire);
}

void service_impl::set_default(const logger_sptr& ptr) {  // NOLINT
  default_logger_.store(ptr, std::memory_order::release);
}

}  // namespace jt::log
