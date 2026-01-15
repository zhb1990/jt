export module jt:log.fwd;

import :detail.memory;
export namespace jt::log {

struct message;
class sink;
class logger;
class service;
struct formatter;

using formatter_ptr = detail::dynamic_unique_ptr<formatter>;
using sink_ptr = detail::dynamic_unique_ptr<sink>;
using logger_sptr = std::shared_ptr<logger>;
using logger_wptr = std::weak_ptr<logger>;

}  // namespace jt::log