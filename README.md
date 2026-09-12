# JT Framework

使用 C++23 modules 的服务器框架基础库，提供基于 mimalloc 的内存管理、缓冲区和同步/异步日志。后续的协程、网络与可选 Actor 服务按独立领域扩展，均依赖日志。

## 当前能力

- `import jt;`：基础与日志便捷入口。
- `import jt.base;`：内存、智能指针、缓冲区、容器及概念，命名空间 `jt::base`。
- `import jt.log;`：logger、service、formatter、控制台与文件 sink，命名空间 `jt::log`。
- 文件日志支持大小/日期轮转、manifest 恢复、LZ4 归档和过期清理；清理仅处理名称及日期/序号结构完整匹配的归档，保留格式不匹配的文件。
- 归档发布不覆盖已有目标；同名冲突或文件系统不支持硬链接时保留源日志并报告错误，已有临时文件也不会被覆盖。
- 协程、网络、Actor 尚未实现；当前没有对应模块或构建开关。

## 构建

需要 CMake 4.3+、Ninja 1.11+、64 位工具链，以及 mimalloc、LZ4、RapidJSON 的 CMake packages。当前不需要 Asio。

工具链必须同时支持 C++23 语言特性和 `import std`。CMake 的 `import std` 支持范围包括 GCC 15+、LLVM Clang 18.1.2+ 与匹配的标准库、MSVC 14.36+；这些是支持门槛，不代表所有组合都已验证。项目还使用 `std::format`、`std::print`、时区数据库及显式对象参数，应以完整构建和测试为准。Apple Clang 不等同于 Homebrew LLVM Clang。

参考：[CMake C++ modules 文档](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html)。实验性 `import std` gate 随 CMake 版本变化；当前默认 gate 在本机 CMake 4.4.3 验证通过，其他版本可通过同名 CMake cache 参数覆盖。

仓库预设默认使用 vcpkg manifest（`vcpkg.json`）安装依赖，并通过 `jt-gcc16` 同时作为 target 与 host triplet，为项目、依赖和 vcpkg host 工具选择 GCC 16。Windows 上因此不需要 Visual Studio / MSVC。先设置 `VCPKG_ROOT` 指向已安装的 vcpkg 根目录：POSIX shell 使用 `export VCPKG_ROOT=/path/to/vcpkg`，PowerShell 使用 `$env:VCPKG_ROOT = 'C:/path/to/vcpkg'`。

Cursor、VS Code CMake Tools 等 GUI 进程可能缺少 `ProgramFiles(x86)`。若仍使用 vcpkg 默认 host triplet `x64-windows`，会在配置阶段报 `Unable to find a valid Visual Studio instance`；请选用仓库的 `debug` / `release` 预设，不要改用 `x64-windows`。

Windows 使用 MSYS2 UCRT64 的 `gcc.exe`/`g++.exe`，默认目录为 `C:/msys64`，可通过 `MSYS2_ROOT` 修改；macOS 通过 `brew --prefix gcc` 定位 Homebrew 的 `gcc-16`/`g++-16`，不会使用 Apple 自带编译器；Linux 从 PATH 查找 `gcc-16`/`g++-16`。预设的链式工具链会设置编译器，仅设置 `CXX` 不会覆盖它。

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# Release 同时构建基准程序。
cmake --preset release
cmake --build --preset release
ctest --preset release
./build/release/benchmarks/jt_log_benchmark
```

另有 `release-with-debug` 配置、构建和测试预设，使用 `RelWithDebInfo` 并启用基准，输出到 `build/release-with-debug`。`jt-gcc16` 在 Windows x64 使用动态依赖，在 macOS/Linux 按宿主架构选择 x64 或 arm64 并使用静态依赖。

C++ 编译的第三方依赖也必须与所选编译器/标准库匹配。特别是 macOS 上，不应将 Apple Clang/libc++ 构建的 mimalloc 静态库直接混入 GCC/libstdc++；Debug 偶然链接成功并不代表 Release 可用。可使用匹配的 vcpkg triplet，或通过 `mimalloc_DIR` 指向同工具链构建的包。

mimalloc 头文件通过普通翻译单元 `src/base/memory_backend.cpp` 隔离，避免 3.3.2 引入的 `<wchar.h>` 等系统声明与 macOS/GCC 的 `import std` 冲突。升级依赖时仍需重新构建和测试；具体版本及平台结果见 [验证记录](docs/validation.md)。

使用其他编译器或已有依赖包时，在独立构建目录手动配置，避免继承预设中的 GCC 16 triplet：

```sh
cmake -S . -B build/custom -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/path/to/compiler -DCMAKE_PREFIX_PATH=/path/to/dependencies
cmake --build build/custom
ctest --test-dir build/custom --output-on-failure
```

也可在首次配置时指定自己的 vcpkg toolchain 和匹配的 triplet。切换工具链应使用新的构建目录。

示例程序位于 `build/debug/examples/main`（Windows 为 `main.exe`）。示例会在工作目录写日志，建议在单独的临时目录运行。保留 `cmake --build build/debug --target main`。

当前选项：`BUILD_TESTING=ON`、`JT_BUILD_EXAMPLES=ON`、`JT_BUILD_BENCHMARKS=OFF`。`release` 和 `release-with-debug` 预设将 benchmarks 打开。

## 消费库


`allocator<T>`、`make_unique<T>` 和 `make_dynamic_unique<Base, Derived>` 按实际对象类型的对齐要求分配内存，支持高对齐类型。原始内存可使用 `allocate(size, alignment)`，alignment 必须为非零的 2 的幂，释放仍调用 `deallocate`。

`jt::base::base_memory_buffer` 支持追加自身已写入区域的完整内容或子区间，例如 `buffer.append(std::string_view(buffer))`，扩容时也能正确复制。追加后若发生扩容，调用者持有的旧指针和视图会失效，应重新获取。

`base_memory_buffer` 支持不同固定容量之间的复制构造和赋值（例如 `buffer_1k` 与 `buffer_2k`），保留读写位置及已写入内容，并按需扩容。

在同一 CMake 构建中使用 `add_subdirectory` 或 FetchContent 引入项目，然后链接 `jt::jt`。该别名对应共享库 `libjt`，公开传递 C++23 编译要求和模块文件集。消费工程同样需要在创建目标前配置 `import std` 支持（包括相应 CMake experimental gate 和 `CMAKE_CXX_MODULE_STD=ON`）。目前不提供安装包，也不分发跨工具链通用 BMI。

```cmake
add_subdirectory(path/to/jt)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE jt::jt)
jt_configure_executable(app)
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

`service` 构造时启动后台线程，`request_stop()` 关闭异步提交，析构时等待写入排空、刷新仍存活的异步 logger 的待刷新 sink，并等待归档排空；无需调用 `start()`，也没有公开的 `wait_stop()`。logger 只通过 `service::create_logger` 创建，返回 `std::shared_ptr<logger>`；日志辅助函数接收 `logger&`。异步 logger 和文件 sink 的归档句柄不延长 service 生命周期。应用应先停止日志生产者，最后销毁日志 service。

`import jt.log.format;` 提供向 `jt::base::buffer_1k` 追加的 `jt::log::format_to(buffer, fmt, args...)` 和 `jt::log::vformat_to(buffer, fmt, format_args)`，也由 `jt.log` 和 `jt` 再导出。前者保留编译期格式串检查，后者接受运行时格式串；二者同步格式化并向调用者传播异常，日志和控制台辅助函数继续捕获异常。运行时格式化引擎及本地时间格式化集中在单个实现单元，避免已复现的 GCC 16.2 / MinGW `import std` 重复定义，不依赖 `--allow-multiple-definition`。用户自己的标准库格式化调用及自定义 `std::formatter` 内部实现仍受工具链限制。

`create_logger` 的范围重载支持迭代器与 sentinel 类型不同的输入范围，逐项移动 sink 并保持顺序。缓冲区跳过操作按剩余长度饱和；请求不可表示的可写容量时抛出 `std::length_error`，保留原内容和读写位置。每日文件轮转包含恰好次日零点的边界。归档线程隔离临时队列分配及单条请求的异常。

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
