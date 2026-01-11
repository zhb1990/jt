// module jt:log.service;
module jt;

import std;
import :log.fwd;
import :log.message;

namespace jt::log {

service::service() {}

service::~service() noexcept {}

auto service::find(std::string_view name) -> logger_sptr { return {}; }

void service::erase(std::string_view name) {}

void service::clear() {}

void service::start() {}

void service::stop() {}

auto service::get_default() -> logger_sptr { return {}; }

void service::set_default(logger_sptr ptr) {}

void service::flush(logger_wptr ptr) {}

void service::log(logger_wptr ptr, std::uint32_t sid, level lv,
                  detail::buffer_1k& buf, const std::source_location& source) {}

}  // namespace jt::log