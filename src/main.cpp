import jt;
import std;

int main(int argc, char** argv) {
    jt::base_memory_buffer<1> buffer;
    buffer.append("hello");
    std::string_view strv(buffer);
    std::println("{}", strv);
    return 0;
}