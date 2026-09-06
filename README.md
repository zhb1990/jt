# JT Framework

使用 C++23 modules 的服务器框架基础库，提供基于 mimalloc 的内存管理、缓冲区和同步/异步日志。后续的协程、网络与可选 Actor 服务按独立领域扩展，均依赖日志。

## 当前能力

- `import jt;`：基础与日志便捷入口。
- `import jt.base;`：内存、智能指针、缓冲区、容器及概念，命名空间 `jt::base`。
- `import jt.log;`：logger、service、formatter、控制台与文件 sink，命名空间 `jt::log`。
- 文件日志支持大小/日期轮转、manifest 恢复、LZ4 归档和过期清理。
- 协程、网络、Actor 尚未实现；当前没有对应模块或构建开关。

## 构建

需要 CMake 4.3+、Ninja 1.11+、64 位工具链，以及 mimalloc、LZ4、RapidJSON 的 CMake packages。当前不需要 Asio。

工具链必须同时支持 C++23 语言特性和 `import std`。CMake 的 `import std` 支持范围包括 GCC 15+、LLVM Clang 18.1.2+ 与匹配的标准库、MSVC 14.36+；这些是支持门槛，不代表所有组合都已验证。项目还使用 `std::format`、`std::print`、时区数据库及显式对象参数，应以完整构建和测试为准。Apple Clang 不等同于 Homebrew LLVM Clang。

参考：[CMake C++ modules 文档](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html)。实验性 `import std` gate 随 CMake 版本变化；当前默认 gate 在本机 CMake 4.4.3 验证通过，其他版本可通过同名 CMake cache 参数覆盖。

```sh
# 首次配置时通过 CXX 选择编译器；依赖可通过 CMAKE_PREFIX_PATH 查找。
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# Release 同时构建基准程序。
cmake --preset release
cmake --build --preset release
ctest --preset release
./build/release/benchmarks/jt_log_benchmark
```

C++ 编译的第三方依赖也必须与所选编译器/标准库匹配。特别是 macOS 上，不应将 Apple Clang/libc++ 构建的 mimalloc 静态库直接混入 GCC/libstdc++；Debug 偶然链接成功并不代表 Release 可用。可使用匹配的 vcpkg triplet，或通过 `mimalloc_DIR` 指向同工具链构建的包。

使用 vcpkg 时，在首次配置追加：

```sh
cmake --preset debug -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

示例程序位于 `build/debug/examples/main`（Windows 为 `main.exe`）。示例会在工作目录写日志，建议在单独的临时目录运行。保留 `cmake --build build/debug --target main`。

当前选项：`BUILD_TESTING=ON`、`JT_BUILD_EXAMPLES=ON`、`JT_BUILD_BENCHMARKS=OFF`。Release preset 将 benchmarks 打开。

## 消费库

在同一 CMake 构建中使用 `add_subdirectory` 或 FetchContent 引入项目，然后链接 `jt::jt`。该别名对应共享库 `libjt`，公开传递 C++23 编译要求和模块文件集。消费工程同样需要在创建目标前配置 `import std` 支持（包括相应 CMake experimental gate 和 `CMAKE_CXX_MODULE_STD=ON`）。目前不提供安装包，也不分发跨工具链通用 BMI。

```cmake
add_subdirectory(path/to/jt)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE jt::jt)
```

```cpp
import std;
import jt;

int main() {
  jt::log::service service;
  auto log = service.create_logger(
      std::array{
          jt::base::make_dynamic_unique<jt::log::sink, jt::log::sink_stdout>()},
      "app", true);
  jt::log::info(*log, "Hello {}", "JT");
  jt::log::vinfo(*log, "allocated bytes: {}", jt::base::allocated_memory());
  service.request_stop();
}
```

logger 只通过 `service::create_logger` 创建，返回 `std::shared_ptr<logger>`；日志辅助函数接收 `logger&`。异步 logger 和文件 sink 的归档句柄不延长 service 生命周期。应用应先停止日志生产者，最后销毁日志 service。

## 目录

| 目录 | 内容 |
|---|---|
| `src/base` | 公开基础模块及实现 |
| `src/detail` | PRIVATE 平台、队列和统计实现 |
| `src/log/core` | logger/service 接口分区及后端 |
| `src/log/sinks` | sink 基类、控制台和文件输出 |
| `examples` | 示例，保留 main 目标 |
| `tests` | 行为、文件、模块消费及拒绝导入测试 |
| `benchmarks` | 日志吞吐、提交延迟和内存占用基准 |
| `cmake` | 模块登记、工具链检查、平台链接规则 |
| `docs` | 架构、迁移、验证记录 |

详见 [架构](docs/architecture.md)、[迁移说明](docs/migration.md)、[验证记录](docs/validation.md) 和 [开发规则](AGENTS.md)。
