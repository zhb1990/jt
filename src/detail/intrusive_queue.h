
#include <cassert>
#include <cstddef>
#include <utility>

namespace jt::detail {

template <auto Next>
class intrusive_queue;

template <typename Node, Node* Node::* Next>
class intrusive_queue<Next> {
  public:
    intrusive_queue() noexcept = default;

    intrusive_queue(intrusive_queue&& other) noexcept
        : head_(std::exchange(other.head_, nullptr)), tail_(std::exchange(other.tail_, nullptr)) {}

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

    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }

    Node* front() const noexcept { return head_; }

    Node* back() const noexcept { return tail_; }

    void clear() noexcept {
        head_ = nullptr;
        tail_ = nullptr;
    }

  private:
    Node* head_{nullptr};
    Node* tail_{nullptr};
};

}  // namespace jt::detail