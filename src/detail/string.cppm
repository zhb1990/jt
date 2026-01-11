export module jt:detail.string;

import std;
import :detail.memory;

export namespace jt::detail {

using string =
    std::basic_string<char, std::char_traits<char>, detail::allocator<char>>;

using wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>,
                                  detail::allocator<wchar_t>>;

}  // namespace jt::detail