module;

#include <cassert>

export module jt:detail.intrusive_queue;

import std;

export namespace jt::detail {

// https://github.com/NVIDIA/stdexec/blob/main/include/stdexec/__detail/__intrusive_queue.hpp
template <auto Next>
class intrusive_queue;

template <typename Node, Node* Node::* Next>
class intrusive_queue<Next> {
 public:
  intrusive_queue() noexcept = default;

  intrusive_queue(intrusive_queue&& other) noexcept
      : head_(std::exchange(other.head_, nullptr)),
        tail_(std::exchange(other.tail_, nullptr)) {}

  intrusive_queue(Node* head, Node* tail) noexcept : head_(head), tail_(tail) {}

  ~intrusive_queue() { assert(empty()); }

  auto operator=(intrusive_queue&& other) noexcept -> intrusive_queue& {
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    return *this;
  }

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

  [[nodiscard]] auto empty() const noexcept -> bool { return head_ == nullptr; }

  void clear() noexcept {
    head_ = nullptr;
    tail_ = nullptr;
  }

  [[nodiscard]]
  auto pop_front() noexcept -> Node* {
    assert(!empty());
    Node* node = std::exchange(head_, head_->*Next);
    if (node->*Next == nullptr) {
      tail_ = nullptr;
    }
    return node;
  }

  void push_front(Node* node) noexcept {
    assert(node != nullptr);
    node->*Next = head_;
    head_ = node;
    if (tail_ == nullptr) {
      tail_ = node;
    }
  }

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

  struct iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = Node*;

    iterator() noexcept = default;

    explicit iterator(Node* pred, Node* node) noexcept
        : predecessor_(pred), node_(node) {}

    [[nodiscard]]
    auto operator*() const noexcept -> Node* {
      assert(node_ != nullptr);
      return node_;
    }

    [[nodiscard]]
    auto operator->() const noexcept -> Node** {
      assert(node_ != nullptr);
      return &node_;
    }

    auto operator++() noexcept -> iterator& {
      predecessor_ = node_;
      if (node_) {
        node_ = node_->*Next;
      }
      return *this;
    }

    auto operator++(int) noexcept -> iterator {
      iterator result = *this;
      ++*this;
      return result;
    }

    friend auto operator==(const iterator&, const iterator&) noexcept
        -> bool = default;

   protected:
    Node* predecessor_ = nullptr;
    Node* node_ = nullptr;
  };

  [[nodiscard]]
  auto begin() const noexcept -> iterator {
    return iterator(nullptr, head_);
  }

  [[nodiscard]]
  auto end() const noexcept -> iterator {
    return iterator(tail_, nullptr);
  }

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

  Node* front() const noexcept { return head_; }

  Node* back() const noexcept { return tail_; }

 private:
  Node* head_{nullptr};
  Node* tail_{nullptr};
};

}  // namespace jt::detail