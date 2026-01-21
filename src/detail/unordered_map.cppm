export module jt:detail.unordered_map;

import std;
import :detail.memory;

export namespace jt::detail {

template <class Key, class T, class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = allocator<std::pair<const Key, T>>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

template <class Key, class T, class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = allocator<std::pair<const Key, T>>>
using unordered_multimap =
    std::unordered_multimap<Key, T, Hash, KeyEqual, Allocator>;

}  // namespace jt::detail