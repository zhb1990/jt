module;

#include "config.h"

export module jt.detail.allocator;

export namespace jt::detail {

JT_API void* allocate(std::size_t size);
JT_API void deallocate(void* ptr, std::size_t size);

}  // namespace jt::detail