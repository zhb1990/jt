module;

#include "config.h"

export module jt.detail.memory;

import std;

export namespace jt::detail {

JT_API auto allocate(std::size_t size) -> void*;

JT_API void deallocate(void* ptr, std::size_t size);

JT_API auto allocated_memory() -> std::map<std::thread::id, std::int64_t>;

}  // namespace jt::detail