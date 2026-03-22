module;

#include <cassert>

export module jt:detail.intrusive_queue;

import std;

export namespace jt::detail {

// https://github.com/NVIDIA/stdexec/blob/main/include/stdexec/__detail/__intrusive_queue.hpp
template <auto Next>
class intrusive_queue;

/**
 * 入侵式队列模板实现
 * 该队列通过在节点中嵌入指针来实现，避免了额外的内存分配
 * @tparam Next 指向节点中下一个节点指针的成员指针
 */
template <typename Node, Node* Node::* Next>
class intrusive_queue<Next> {
 public:
  /**
   * 默认构造函数
   * 创建一个空队列
   */
  intrusive_queue() noexcept = default;

  /**
   * 移动构造函数
   * @param other 要移动的队列
   */
  intrusive_queue(intrusive_queue&& other) noexcept
      : head_(std::exchange(other.head_, nullptr)),
        tail_(std::exchange(other.tail_, nullptr)) {}

  /**
   * 从头尾节点构造队列
   * @param head 队列头节点
   * @param tail 队列尾节点
   */
  intrusive_queue(Node* head, Node* tail) noexcept : head_(head), tail_(tail) {}

  /**
   * 析构函数
   * 断言队列为空（在调试模式下）
   */
  ~intrusive_queue() { assert(empty()); }

  /**
   * 移动赋值运算符
   * @param other 要赋值的队列
   * @return 赋值后的队列引用
   */
  auto operator=(intrusive_queue&& other) noexcept -> intrusive_queue& {
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    return *this;
  }

  /**
   * 创建反转的队列
   * @param list 要反转的节点列表
   * @return 反转后的队列
   */
  static auto make_reversed(Node* list) noexcept -> intrusive_queue {
    Node* new_head = nullptr;
    Node* new_tail = list;
    while (list != nullptr) {
      Node* next = list->*Next;
      list->*Next = new_head;
      new_head = list;
      list = next;
    }

    return intrusive_queue(new_head, new_tail);
  }

  /**
   * 创建队列
   * @param list 要转换为队列的节点列表
   * @return 包含所有节点的队列
   */
  static auto make(Node* list) noexcept -> intrusive_queue {
    intrusive_queue result(list, list);
    if (list == nullptr) {
      return result;
    }

    while (result.tail_->*Next != nullptr) {
      result.tail_ = result.tail_->*Next;
    }

    return result;
  }

  /**
   * 检查队列是否为空
   * @return 如果队列为空返回true，否则返回false
   */
  [[nodiscard]] auto empty() const noexcept -> bool { return head_ == nullptr; }

  /**
   * 清空队列
   * 注意：这不会释放节点内存，只会将队列设置为空状态
   */
  void clear() noexcept {
    head_ = nullptr;
    tail_ = nullptr;
  }

  /**
   * 从队列前端弹出节点
   * @return 弹出的节点指针，如果队列为空则行为未定义（通过断言保护）
   */
  [[nodiscard]]
  auto pop_front() noexcept -> Node* {
    assert(!empty());
    Node* node = std::exchange(head_, head_->*Next);
    if (node->*Next == nullptr) {
      tail_ = nullptr;
    }
    return node;
  }

  /**
   * 在队列前端推入节点
   * @param node 要推入的节点指针（不能为空）
   */
  void push_front(Node* node) noexcept {
    assert(node != nullptr);
    node->*Next = head_;
    head_ = node;
    if (tail_ == nullptr) {
      tail_ = node;
    }
  }

  /**
   * 在队列尾端推入节点
   * @param node 要推入的节点指针（不能为空）
   */
  void push_back(Node* node) noexcept {
    assert(node != nullptr);
    node->*Next = nullptr;
    if (tail_ == nullptr) {
      head_ = node;
    } else {
      tail_->*Next = node;
    }
    tail_ = node;
  }

  /**
   * 在队列末尾追加另一个队列
   * @param other 要追加的队列（操作后other将变为空）
   */
  void append(intrusive_queue other) noexcept {
    if (other.empty()) return;

    auto* other_head = std::exchange(other.head_, nullptr);
    if (empty()) {
      head_ = other_head;
    } else {
      tail_->*Next = other_head;
    }
    tail_ = std::exchange(other.tail_, nullptr);
  }

  /**
   * 在队列开头前置另一个队列
   * @param other 要前置的队列（操作后other将变为空）
   */
  void prepend(intrusive_queue other) noexcept {
    if (other.empty()) return;

    other.tail_->*Next = head_;
    head_ = other.head_;
    if (tail_ == nullptr) {
      tail_ = other.tail_;
    }

    other.tail_ = nullptr;
    other.head_ = nullptr;
  }

  /**
   * 队列迭代器
   * 用于遍历队列中的节点
   */
  struct iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = Node*;

    iterator() noexcept = default;

    /**
     * 构造迭代器
     * @param pred 前驱节点
     * @param node 当前节点
     */
    explicit iterator(Node* pred, Node* node) noexcept
        : predecessor_(pred), node_(node) {}

    /**
     * 解引用操作符
     * @return 当前节点指针
     */
    [[nodiscard]]
    auto operator*() const noexcept -> Node* {
      assert(node_ != nullptr);
      return node_;
    }

    /**
     * 成员访问操作符
     * @return 当前节点指针的地址
     */
    [[nodiscard]]
    auto operator->() const noexcept -> Node** {
      assert(node_ != nullptr);
      return &node_;
    }

    /**
     * 前置递增操作符
     * @return 递增后的迭代器引用
     */
    auto operator++() noexcept -> iterator& {
      predecessor_ = node_;
      if (node_) {
        node_ = node_->*Next;
      }
      return *this;
    }

    /**
     * 后置递增操作符
     * @return 递增前的迭代器副本
     */
    auto operator++(int) noexcept -> iterator {
      iterator result = *this;
      ++*this;
      return result;
    }

    /**
     * 相等比较操作符
     */
    friend auto operator==(const iterator&, const iterator&) noexcept
        -> bool = default;

   protected:
    Node* predecessor_ = nullptr;
    Node* node_ = nullptr;
  };

  /**
   * 获取指向队列头部的迭代器
   * @return 指向队列头部的迭代器（如果队列为空则指向尾部）
   */
  [[nodiscard]]
  auto begin() const noexcept -> iterator {
    return iterator(nullptr, head_);
  }

  /**
   * 获取指向队列尾部的迭代器
   * @return 指向队列尾部的迭代器
   */
  [[nodiscard]]
  auto end() const noexcept -> iterator {
    return iterator(tail_, nullptr);
  }

  /**
   * 截取另一队列的一段并插入到指定位置
   * @param pos 插入位置
   * @param other 来源队列
   * @param first 要截取的第一个节点
   * @param last 要截取的最后一个节点（不包含）
   */
  void splice(iterator pos, intrusive_queue& other, iterator first,
              iterator last) noexcept {
    if (first == last) {
      return;
    }

    assert(first.node_ != nullptr);
    assert(last.predecessor_ != nullptr);
    if (other.head_ == first.node_) {
      other.head_ = last.node_;
      if (other.head_ == nullptr) {
        other.tail_ = nullptr;
      }
    } else {
      assert(first.predecessor_ != nullptr);
      first.predecessor_->*Next = last.node_;
      last.predecessor_->*Next = pos.node_;
    }
    if (empty()) {
      head_ = first.node_;
      tail_ = last.predecessor_;
    } else {
      pos.predecessor_->*Next = first.node_;
      if (pos.node_ == nullptr) {
        tail_ = last.predecessor_;
      }
    }
  }

  /**
   * 获取队列头节点
   * @return 队列头节点指针，如果队列为空返回nullptr
   */
  Node* front() const noexcept { return head_; }

  /**
   * 获取队列尾节点
   * @return 队列尾节点指针，如果队列为空返回nullptr
   */
  Node* back() const noexcept { return tail_; }

 private:
  Node* head_{nullptr};
  Node* tail_{nullptr};
};

}  // namespace jt::detail