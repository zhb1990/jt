module;

#include "config.h"

export module jt.detail.memory;

export namespace jt::detail {

JT_API auto allocate(std::size_t size) -> void*;
JT_API void deallocate(void* ptr, std::size_t size);

}  // namespace jt::detail