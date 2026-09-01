import jt;
import std;

/**
 * 主程序入口
 * 演示 JT 框架的主要功能：
 * 1. 内存管理（分配、跟踪）
 * 2. 日志系统（同步/异步、文件轮换、控制台输出）
 * 3. 缓冲区操作
 */
struct test_node {
  std::int64_t value{0};
  test_node* next{nullptr};
};

int main(int argc, char** argv) {
  // 打印初始内存使用情况
  std::println("mem {}", jt::detail::allocated_memory());
  {
    // 配置文件日志Sink
    jt::log::sink_file_config config;
    config.daily_rotation = true;    // 启用每日轮换
    config.directory = "./";         // 日志文件目录
    config.keep_days = 1;            // 保留1天
    config.lz4_directory = "./lz4";  // 压缩日志目录
    config.max_size = 1024;          // 最大文件大小（小值便于测试）
    config.name = "test";            // 日志文件基础名称

    // 创建日志服务
    jt::log::service service;

    // 创建两个Sink：文件Sink和标准输出Sink
    std::array a1{
        jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_file>(
            service, config),
        jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_stdout>()};

    std::this_thread::sleep_for(std::chrono::seconds(1));
    // 创建异步日志记录器
    const auto log_ptr = service.create_logger(std::move(a1), "test", true);
    auto& log = *log_ptr;
    jt::log::info(log, "mem {}", jt::detail::allocated_memory());

    // 测试固定大小缓冲区
    jt::detail::base_memory_buffer<1> b1;
    b1.shrink();  // 收缩缓冲区
    jt::log::info(log, "readable {}", b1.readable());

    {
      // 测试缓冲区操作
      jt::detail::base_memory_buffer<1> buffer;
      buffer.append("hello");  // 追加字符串
      std::string_view strv(buffer);
      jt::log::info(log, "{}", strv);  // 输出: hello

      std::format_to(std::back_inserter(buffer), " {}", "world");  // 格式化追加
      std::string_view strv1(buffer);
      jt::log::info(log, "{}", strv1);  // 输出: hello world

      jt::log::info(log, "mem {}", jt::detail::allocated_memory());
      const jt::detail::read_buffer rb(buffer);  // 转换为只读缓冲区
      jt::log::info(log, "{}", std::string_view(rb));
    }
    jt::log::info(log, "mem {}", jt::detail::allocated_memory());

    {
      // 测试原始内存分配
      void* ptr = jt::detail::allocate(100);
      jt::log::info(log, "mem {}", jt::detail::allocated_memory());
      jt::log::info(log, "ptr size {}", jt::detail::allocated_size(ptr));
      jt::detail::deallocate(ptr);  // 释放内存
      jt::log::info(log, "mem {}", jt::detail::allocated_memory());
    }

    // 使用变量参数记录警告日志（中文）
    const std::string fmt = "使用的内存 {}";
    jt::log::vwarn(log, fmt, jt::detail::allocated_memory());
    std::println("mem {}", jt::detail::allocated_memory());
  }
  std::println("mem {}", jt::detail::allocated_memory());

  return 0;
}