module;

#include <cassert>

export module jt:detail.intrusive_mpsc_queue;

import std;
import :detail.intrusive_queue;
import :detail.cpu_pause;

export namespace jt::detail {

// https://github.com/NVIDIA/stdexec/blob/main/include/stdexec/__detail/__intrusive_mpsc_queue.hpp
template <auto Next>
class intrusive_mpsc_queue;

template <class Node, std::atomic<void*> Node::* Next>
class intrusive_mpsc_queue<Next> {
  std::atomic<Node*> nil_ = nullptr;
  std::atomic<void*> back_{&nil_};
  void* front_{&nil_};

  void push_back_nil() {
    nil_.store(nullptr, std::memory_order_relaxed);
    auto* prev =
        static_cast<Node*>(back_.exchange(&nil_, std::memory_order_acq_rel));
    (prev->*Next).store(&nil_, std::memory_order_release);
  }

 public:
  auto push_back(Node* new_node) noexcept -> bool {
    (new_node->*Next).store(nullptr, std::memory_order_relaxed);
    void* prev_back = back_.exchange(new_node, std::memory_order_acq_rel);
    bool is_nil = prev_back == static_cast<void*>(&nil_);
    if (is_nil) {
      nil_.store(new_node, std::memory_order_release);
    } else {
      (static_cast<Node*>(prev_back)->*Next)
          .store(new_node, std::memory_order_release);
    }
    return is_nil;
  }

  auto pop_front() noexcept -> Node* {
    if (front_ == static_cast<void*>(&nil_)) {
      Node* next = nil_.load(std::memory_order_acquire);
      if (!next) {
        return nullptr;
      }
      front_ = next;
    }
    auto* front = static_cast<Node*>(front_);
    void* next = (front->*Next).load(std::memory_order_acquire);
    if (next) {
      front_ = next;
      return front;
    }
    assert(!next);
    push_back_nil();
    do {
      cpu_pause();
      next = (front->*Next).load(std::memory_order_acquire);
    } while (!next);
    front_ = next;
    return front;
  }
};

}  // namespace jt::detail