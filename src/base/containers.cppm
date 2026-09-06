export module jt.base.containers;

import std;
import jt.base.memory;

export namespace jt::base {

template <typename T>
using vector = std::vector<T, allocator<T>>;

using string = std::basic_string<char, std::char_traits<char>, allocator<char>>;

using wstring =
    std::basic_string<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t>>;

template <class T, class Allocator = allocator<T>>
using deque = std::deque<T, Allocator>;

template <class Key, class T, class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = allocator<std::pair<const Key, T>>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

template <class Key, class T, class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = allocator<std::pair<const Key, T>>>
using unordered_multimap =
    std::unordered_multimap<Key, T, Hash, KeyEqual, Allocator>;

}  // namespace jt::base
