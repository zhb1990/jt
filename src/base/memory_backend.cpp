#include "memory_backend.h"

#include <mimalloc.h>

// This ordinary translation unit must not import std or any JT module.
// mimalloc's transitive system headers conflict with GCC's std module on macOS.
namespace jt::detail {

auto memory_allocate(memory_size size, memory_size alignment) noexcept
    -> void* {
  return mi_malloc_aligned(size, alignment);
}

auto memory_usable_size(const void* ptr) noexcept -> memory_size {
  return mi_usable_size(ptr);
}

void memory_free(void* ptr) noexcept { mi_free(ptr); }

}  // namespace jt::detail
