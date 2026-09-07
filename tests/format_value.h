#pragma once

// Both consumers import std before this header. Exercise a user formatter
// without invoking another runtime format engine inside the callback.
struct format_value {
  std::string_view text;
};

template <>
struct std::formatter<format_value> {
  constexpr auto parse(std::format_parse_context& context) {
    auto it = context.begin();
    if (it != context.end() && *it != '}') {
      throw std::format_error("unexpected format specifier");
    }
    return it;
  }

  auto format(const format_value& value, std::format_context& context) const {
    auto out = context.out();
    for (const auto ch : value.text) *out++ = ch;
    return out;
  }
};
