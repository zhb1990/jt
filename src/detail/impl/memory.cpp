module;

#include <mimalloc.h>

// module jt:detail.memory;
module jt;

import :detail.metric_value;

namespace jt::detail {
metric_value memory_total;

auto allocate(const std::size_t size) -> void* {
  void* ptr = mi_malloc(size);
  const auto real = mi_usable_size(ptr);
  memory_total.fetch_add(real);
  return ptr;
}

auto allocated_size(const void* ptr) -> std::size_t {
  return mi_usable_size(ptr);
}

void deallocate(void* ptr) {
  const auto real = mi_usable_size(ptr);
  memory_total.fetch_sub(real);
  return mi_free(ptr);
}

auto allocated_memory() -> std::int64_t { return memory_total.count(); }

}  // namespace jt::detail