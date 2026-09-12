module;

#include "memory_backend.h"

module jt.base.memory;

import std;
import jt.detail.metric_value;

namespace jt::base {

using detail::metric_value;

/**
 * 全局内存统计对象
 * 用于跟踪当前进程分配的总内存量
 */
auto memory_total() -> metric_value& {
  struct alignas(metric_value) storage_type {
    std::byte bytes[sizeof(metric_value)];
  };

  static storage_type storage{};
  static metric_value* const instance =
      ::new (static_cast<void*>(storage.bytes)) metric_value();

  return *instance;
}

/**
 * 分配指定大小的内存块
 * 使用mimalloc库进行内存分配，并更新内存统计
 * @param size 要分配的字节数
 * @return 分配的内存指针；失败时抛出 std::bad_alloc
 */
auto allocate(const std::size_t size) -> void* {
  return allocate(size, alignof(std::max_align_t));
}

auto allocate(const std::size_t size, const std::size_t alignment) -> void* {
  if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
    throw std::bad_alloc();
  }
  
  auto& metric = memory_total();
  void* ptr = detail::memory_allocate(size, alignment);
  if (!ptr) {
    throw std::bad_alloc();  // 如果分配失败，抛出异常
  }

  // 获取实际分配的大小（可能包括额外的管理开销）
  const auto real = detail::memory_usable_size(ptr);
  metric.fetch_add(real);  // 更新内存统计
  return ptr;
}

/**
 * 获取指定内存块的实际大小
 * @param ptr 内存指针
 * @return 实际分配的字节数
 */
auto allocated_size(const void* ptr) -> std::size_t {
  return detail::memory_usable_size(ptr);  // 使用mimalloc获取实际分配大小
}

/**
 * 释放内存块
 * 使用mimalloc库释放内存，并更新内存统计
 * @param ptr 要释放的内存指针
 */
void deallocate(void* ptr) {
  const auto real = detail::memory_usable_size(ptr);  // 获取实际分配的大小
  memory_total().fetch_sub(real);   // 更新内存统计（减去释放的内存）
  return detail::memory_free(ptr);  // 使用mimalloc释放内存
}

/**
 * 获取当前已分配的总内存量
 * @return 已分配的内存字节数（带符号的64位整数）
 */
auto allocated_memory() -> std::int64_t {
  return memory_total().count();  // 返回内存统计对象的当前计数值
}

}  // namespace jt::base
