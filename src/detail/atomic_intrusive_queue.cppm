/**
 * 原子侵入式队列实现
 * 基于NVIDIA stdexec库的线程安全队列实现
 * 使用模板模板参数定义具有next指针的节点类型
 */

module;

#include <cassert>

export module jt:detail.atomic_intrusive_queue;

import std;
import :detail.cache_line;
import :detail.intrusive_queue;

export namespace jt::detail {

/**
 * 原子侵入式队列模板
 * @tparam Next 节点中指向下一个节点的成员指针
 * @see https://github.com/NVIDIA/stdexec/blob/main/include/stdexec/__detail/__atomic_intrusive_queue.hpp
 */
template <auto Next>
class atomic_intrusive_queue;

/**
 * 原子侵入式队列特化实现
 * @tparam Node 节点类型
 * @tparam Next 节点中指向下一个节点的成员指针
 * @note 使用cache_line对齐以避免伪共享
 */
template <typename Node, Node* Node::* Next>
class alignas(cache_line_bytes) atomic_intrusive_queue<Next> {
 public:
  /**
   * 将节点推入队列
   * @param node 要推入的节点指针
   * @return 如果队列之前为空则返回false，否则返回true
   * @note 使用内存顺序acq_rel确保正确的同步
   */
  auto push(Node* node) noexcept -> bool {
    assert(node);
    Node* old_head = head_.load(std::memory_order_relaxed);
    do {
      node->*Next = old_head;
    } while (!head_.compare_exchange_weak(old_head, node,
                                          std::memory_order_acq_rel));
    if (!old_head) {
      return false;
    }

    head_.notify_one();
    return true;
  }

  /**
   * 等待队列中有项目可用
   * @note 使用原子等待操作高效地等待项目
   */
  void wait_for_item() noexcept {
    // 等待直到队列中有项目:
    head_.wait(nullptr);
  }

  /**
   * 弹出队列中的所有项目
   * @return 包含所有项目的入侵队列（已反转以保持FIFO顺序）
   */
  [[nodiscard]] auto pop_all() noexcept -> intrusive_queue<Next> {
    auto* const list = head_.exchange(nullptr, std::memory_order_acquire);
    return intrusive_queue<Next>::make_reversed(list);
  }

 private:
  /** 指向队列头部的原子指针 */
  std::atomic<Node*> head_{nullptr};
};

}  // namespace jt::detail