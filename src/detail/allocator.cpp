module jt.detail.allocator;

import std;

namespace jt::detail {

void* allocate(std::size_t size) { return std::malloc(size); }

void deallocate(void* ptr, std::size_t size) { return std::free(ptr); }

}  // namespace jt::detail
