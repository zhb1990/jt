module;

#include <cassert>

export module jt:detail.intrusive_mpsc_queue;

import std;

export namespace jt::detail {

/**
 * 无锁多生产者单消费者(MPSC)队列
 * 实现基于 NVIDIA stdexec 的 intrusive_mpsc_queue
 * 一般设计来自:
 * https://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue
 */
template <auto Next>
class intrusive_mpsc_queue;

/**
 * 无锁多生产者单消费者(MPSC)队列
 * 使用 intrusive 方式存储节点，通过成员指针模板参数配置节点的 next 成员
 * Node 必须可默认构造，仅用于队列内部 stub 节点；内部只访问 Next 数据成员
 * @tparam Next 指向节点中下一个节点指针的成员指针
 */
template <class Node, std::atomic<Node*> Node::* Next>
  requires std::default_initializable<Node>
class intrusive_mpsc_queue<Next> {
  std::atomic<Node*> head_{&stub_};
  Node* tail_{&stub_};
  Node stub_{};

 public:
  intrusive_mpsc_queue() {
    (stub_.*Next).store(nullptr, std::memory_order_release);
  }

  /**
   * 将节点添加到队列尾部
   * @param new_node 新节点指针
   * @return 如果队列之前为空返回 true，否则返回 false
   */
  auto push_back(Node* new_node) noexcept -> bool {
    (new_node->*Next).store(nullptr, std::memory_order_relaxed);
    Node* prev = head_.exchange(new_node, std::memory_order_acq_rel);
    (prev->*Next).store(new_node, std::memory_order_release);
    return prev == &stub_;
  }

  /**
   * 从队列头部取出节点
   * @return 队列头部节点指针，如果队列为空或生产者正在插入则返回 nullptr
   */
  auto pop_front() noexcept -> Node* {
    Node* tail = this->tail_;
    assert(tail != nullptr);
    Node* next = (tail->*Next).load(std::memory_order_acquire);
    // 若 tail 指向 stub 节点，需要再前进一次
    if (&stub_ == tail) {
      if (nullptr == next) {
        return nullptr;
      }
      this->tail_ = next;
      tail = next;
      next = (next->*Next).load(std::memory_order_acquire);
    }
    // 常见情况：存在下一个节点，直接前进 tail
    if (nullptr != next) {
      this->tail_ = next;
      return tail;
    }
    // next 为 nullptr 表示：
    // 1) 队列中没有更多节点
    // 2) 某个生产者正在插入新节点
    Node const* head = this->head_.load(std::memory_order_acquire);
    // 生产者正在插入新节点，此时不能返回 tail，因为还无法链接下一个节点
    if (tail != head) {
      return nullptr;
    }
    // 队列已空，插入 stub 以便链接到空状态（或后续新节点）
    push_back(&stub_);
    // 重新尝试加载 next
    next = (tail->*Next).load(std::memory_order_acquire);
    if (nullptr != next) {
      // 已成功链接新节点或 stub 节点
      this->tail_ = next;
      return tail;
    }
    // next 仍为 nullptr 且不是 stub，说明生产者正在插入新节点，尚无法链接
    return nullptr;
  }
};

}  // namespace jt::detail
