module;

#include <cstdlib>

module jt.buffer;

namespace jt::impl {

void* buffer_allocate(size_t size) { return std::malloc(size); }

void buffer_deallocate(void* ptr, size_t size) { return std::free(ptr); }

}  // namespace jt::impl