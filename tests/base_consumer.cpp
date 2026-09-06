import std;
import jt.base.memory;
import jt.base.buffer;
import jt.base.containers;
import jt.base.concepts;

static_assert(jt::base::writable_buffer<jt::base::buffer_1k>);
static_assert(!jt::base::writable_buffer<int>);
static_assert(std::same_as<jt::base::vector<int>::allocator_type,
                           jt::base::allocator<int>>);
static_assert(
    std::same_as<jt::base::string::allocator_type, jt::base::allocator<char>>);

int main() {
  jt::base::vector<int> vector{1, 2};
  jt::base::string string = "base";
  jt::base::wstring wide = L"base";
  jt::base::deque<int> deque{3};
  jt::base::unordered_map<int, int> map{{1, 2}};
  jt::base::unordered_multimap<int, int> multimap{{1, 2}, {1, 3}};
  jt::base::buffer_1k buffer;
  std::format_to(std::back_inserter(buffer), "{} {}", string, vector.size());
  auto number = jt::base::make_unique<int>(42);
  return std::string_view(buffer) == "base 2" && *number == 42 &&
                 deque.front() == 3 && map.at(1) == 2 &&
                 multimap.count(1) == 2 && wide.size() == 4
             ? 0
             : 1;
}
