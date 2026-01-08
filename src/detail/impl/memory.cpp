module;

#include <cassert>

// module jt:detail.memory;
module jt;

import :detail.metric_value;

namespace jt::detail {
metric_value memory_total;

auto allocate(const std::size_t size) -> void* {
  const std::size_t real = size + sizeof(std::size_t);
  void* ptr = std::malloc(real);
  *static_cast<std::size_t*>(ptr) = size;
  memory_total.fetch_add(real);
  return static_cast<char*>(ptr) + sizeof(std::size_t);
}

auto allocated_size(void* ptr) -> std::size_t {
  ptr = static_cast<void*>(static_cast<char*>(ptr) - sizeof(std::size_t));
  return *static_cast<std::size_t*>(ptr);
}

void deallocate(void* ptr, const std::size_t size) {
  const std::size_t real = size + sizeof(std::size_t);
  memory_total.fetch_sub(real);
  ptr = static_cast<void*>(static_cast<char*>(ptr) - sizeof(std::size_t));
  assert(size == 0 || *static_cast<std::size_t*>(ptr) == size);
  return std::free(ptr);
}

auto allocated_memory() -> std::int64_t { return memory_total.count(); }

}  // namespace jt::detail