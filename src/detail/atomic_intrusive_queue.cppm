module;

#include <cassert>

export module jt.detail.atomic_intrusive_queue;

import std;
import jt.detail.cache_line;
import jt.detail.intrusive_queue;

export namespace jt::detail {

// https://github.com/NVIDIA/stdexec/blob/main/include/stdexec/__detail/__atomic_intrusive_queue.hpp
template <auto Next>
class atomic_intrusive_queue;

template <typename Node, Node* Node::* Next>
class alignas(cache_line_bytes) atomic_intrusive_queue<Next> {
 public:
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

  void wait_for_item() noexcept {
    // Wait until the queue has an item in it:
    head_.wait(nullptr);
  }

  [[nodiscard]] auto pop_all() noexcept -> intrusive_queue<Next> {
    auto* const list = head_.exchange(nullptr, std::memory_order_acquire);
    return intrusive_queue<Next>::make_reversed(list);
  }

 private:
  std::atomic<Node*> head_{nullptr};
};

}  // namespace jt::detail
