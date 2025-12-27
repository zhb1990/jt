module jt.detail.allocator;

import std;

namespace jt::detail {

auto allocate(std::size_t size) -> void* { return std::malloc(size); }

void deallocate(void* ptr, std::size_t size) { return std::free(ptr); }

}  // namespace jt::detail
