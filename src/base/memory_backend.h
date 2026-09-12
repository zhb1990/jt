#pragma once

// Keep this bridge free of system headers: it is included in a global module
// fragment before import std. sizeof yields the same type as std::size_t.
namespace jt::detail {

using memory_size = decltype(sizeof(0));

auto memory_allocate(memory_size size, memory_size alignment) noexcept -> void*;
auto memory_usable_size(const void* ptr) noexcept -> memory_size;
void memory_free(void* ptr) noexcept;

}  // namespace jt::detail
