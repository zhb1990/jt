module jt:detail.deque;

import std;
import :detail.memory;

namespace jt::detail {

template <class T, class Allocator = allocator<T>>
using deque = std::deque<T, Allocator>;

}  // namespace jt::detail