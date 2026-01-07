module;

#include <cassert>

#include "config.h"

export module jt:detail.memory;

import :detail.metric_value;
import std;

namespace jt::detail {

metric_value memory_total;

export JT_API auto allocate(const std::size_t size) -> void* {
  const std::size_t real = size + sizeof(std::size_t);
  void* ptr = std::malloc(real);
  *static_cast<std::size_t*>(ptr) = size;
  memory_total.fetch_add(real);
  return static_cast<char*>(ptr) + sizeof(std::size_t);
}

export JT_API void deallocate(void* ptr, const std::size_t size) {
  const std::size_t real = size + sizeof(std::size_t);
  memory_total.fetch_sub(real);
  ptr = static_cast<void*>(static_cast<char*>(ptr) - sizeof(std::size_t));
  assert(size == 0 || *static_cast<std::size_t*>(ptr) == size);
  return std::free(ptr);
}

export JT_API auto allocated_memory() -> std::int64_t {
  return memory_total.count();
}

}  // namespace jt::detail
