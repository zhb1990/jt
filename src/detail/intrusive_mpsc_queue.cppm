module;
 
#include <cassert>
 
export module jt:detail.intrusive_mpsc_queue;
 
import std;
import :detail.intrusive_queue;
import :detail.cpu_pause;
 
export namespace jt::detail {
 
/**
 * 无锁多生产者单消费者(MPSC)环形队列
 * 实现基于NVIDIA stdexec库的intrusive_mpsc_queue
 */
template <auto Next>
class intrusive_mpsc_queue;
 
/**
 * 无锁多生产者单消费者(MPSC)环形队列
 * 使用intrusive方式存储节点，通过成员指针模板参数配置节点的_next成员
 * @tparam Next 指向节点中下一个节点指针的成员指针
 */
template <class Node, std::atomic<void*> Node::* Next>
class intrusive_mpsc_queue<Next> {
  std::atomic<Node*> nil_ = nullptr;
  std::atomic<void*> back_{&nil_};
  void* front_{&nil_};
 
  /**
   * 将nil节点添加到队列尾部
   * 用于pop_front()时队列为空的退化情况处理
   */
  void push_back_nil() {
    nil_.store(nullptr, std::memory_order_relaxed);
    auto* prev =
        static_cast<Node*>(back_.exchange(&nil_, std::memory_order_acq_rel));
    (prev->*Next).store(&nil_, std::memory_order_release);
  }
 
 public:
  /**
   * 将节点添加到队列尾部
   * @param new_node 新节点指针
   * @return 如果队列之前为空返回true，否则返回false
   */
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
 
  /**
   * 从队列头部取出节点
   * @return 队列头部节点指针，如果队列为空返回nullptr
   */
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