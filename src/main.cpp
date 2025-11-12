import jt;
import std;

int main(int argc, char** argv) {
    jt::detail::buffer_1k buffer;
    buffer.append("hello");
    std::string_view strv(buffer);
    std::println("{}", strv);
    std::format_to(std::back_inserter(buffer), " {}", "world");
    std::string_view strv1(buffer);
    std::println("{}", strv1);
    return 0;
}