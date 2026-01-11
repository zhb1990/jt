export module jt:detail.vector;

import std;
import :detail.memory;

export namespace jt::detail {

template <typename T>
using vector = std::vector<T, detail::allocator<T>>;

}  // namespace jt::detail