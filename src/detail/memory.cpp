module;

#include "config.h"

module jt.detail.memory;

namespace jt::detail {

namespace {

std::mutex allocated_memory_mutex;
std::map<std::thread::id, std::atomic_int64_t> allocated_memory_map;

std::atomic_int64_t* get_thread_allocated_memory() {
  thread_local std::atomic_int64_t* thread_allocated_memory = nullptr;

  if (!thread_allocated_memory) {
    std::unique_lock lock(allocated_memory_mutex);
    thread_allocated_memory = &allocated_memory_map[std::this_thread::get_id()];
  }

  return thread_allocated_memory;
}

}  // namespace

JT_API auto allocate(std::size_t size) -> void* {
  auto* allocated = get_thread_allocated_memory();
  size += sizeof(void*);
  allocated->fetch_add(size, std::memory_order::relaxed);
  void* ptr = std::malloc(size);
  *static_cast<void**>(ptr) = allocated;
  return static_cast<void*>(static_cast<char*>(ptr) + sizeof(void*));
}

JT_API void deallocate(void* ptr, std::size_t size) {
  size += sizeof(void*);
  ptr = static_cast<void*>(static_cast<char*>(ptr) - sizeof(void*));
  auto* allocated =
      static_cast<std::atomic_int64_t*>(*static_cast<void**>(ptr));
  allocated->fetch_sub(size, std::memory_order::relaxed);
  return std::free(ptr);
}

JT_API auto allocated_memory() -> std::map<std::thread::id, std::int64_t> {
  std::unique_lock lock(allocated_memory_mutex);
  std::map<std::thread::id, std::int64_t> result;
  for (auto& [id, allocated] : allocated_memory_map) {
    result.emplace(id, allocated.load(std::memory_order::relaxed));
  }
  return result;
}

}  // namespace jt::detail
