
#include <cassert>
#include <cstddef>
#include <utility>

namespace jt::detail {

template <auto Next>
class intrusive_queue;

template <typename Node, Node* Node::* Next>
class intrusive_queue<Next> {};

}  // namespace jt::detail