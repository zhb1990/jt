module;

#include "config.h"

export module jt:detail.os;

import std;

export namespace jt::detail {

[[nodiscard]] JT_API int pid();

[[nodiscard]] JT_API std::uint64_t tid();

[[nodiscard]] JT_API const std::error_category& system_category() noexcept;

[[nodiscard]] JT_API std::filesystem::path program_location(
    std::error_code& ec);

}  // namespace jt::detail
