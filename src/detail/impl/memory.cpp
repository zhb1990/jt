module;

#include <mimalloc.h>

// module jt:detail.memory;
module jt;

import :detail.metric_value;

namespace jt::detail {

/**
 * 全局内存统计对象
 * 用于跟踪当前进程分配的总内存量
 */
metric_value memory_total;

/**
 * 分配指定大小的内存块
 * 使用mimalloc库进行内存分配，并更新内存统计
 * @param size 要分配的字节数
 * @return 分配的内存指针，如果分配失败返回nullptr
 */
auto allocate(const std::size_t size) -> void* {
  void* ptr = mi_malloc(size);  // 使用mimalloc分配内存
  const auto real =
      mi_usable_size(ptr);       // 获取实际分配的大小（可能包括额外的管理开销）
  memory_total.fetch_add(real);  // 更新内存统计
  return ptr;
}

/**
 * 获取指定内存块的实际大小
 * @param ptr 内存指针
 * @return 实际分配的字节数
 */
auto allocated_size(const void* ptr) -> std::size_t {
  return mi_usable_size(ptr);  // 使用mimalloc获取实际分配大小
}

/**
 * 释放内存块
 * 使用mimalloc库释放内存，并更新内存统计
 * @param ptr 要释放的内存指针
 */
void deallocate(void* ptr) {
  const auto real = mi_usable_size(ptr);  // 获取实际分配的大小
  memory_total.fetch_sub(real);           // 更新内存统计（减去释放的内存）
  return mi_free(ptr);                    // 使用mimalloc释放内存
}

/**
 * 获取当前已分配的总内存量
 * @return 已分配的内存字节数（带符号的64位整数）
 */
auto allocated_memory() -> std::int64_t {
  return memory_total.count();  // 返回内存统计对象的当前计数值
}

}  // namespace jt::detail