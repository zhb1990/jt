module;

#include <cstdlib>

module jt.detail.allocator;

namespace jt::detail {

void* allocate(size_t size) { return std::malloc(size); }

void deallocate(void* ptr, size_t size) { return std::free(ptr); }

}  // namespace jt::detail