import jt;
import std;

int main(int argc, char** argv) {
    jt::detail::buffer_1k buffer;
    buffer.append("hello");
    std::string_view strv(buffer);
    std::println("{}", strv);
    return 0;
}