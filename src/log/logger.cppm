module;

#include "../detail/config.h"

export module jt:log.logger;

import std;

export namespace jt::log {

class JT_API logger: public std::enable_shared_from_this<logger> {};

}  // namespace jt::log