# JT 框架说明文档

> C++23 模块化游戏服务器框架 | 基于 mimalloc 的高性能内存管理

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-4.3.0+-blue.svg)](https://cmake.org/)

## 项目概述

JT 是一个现代 C++23 编写的轻量级服务器框架，参考了 skynet 的设计理念，采用 C++23 模块化架构。项目专注于高性能、低延迟的服务器开发，特别适用于游戏服务器后端场景。

**核心特性**：
- 🚀 基于 mimalloc 的高性能内存管理
- 📝 高性能日志系统（异步写入、文件轮转、LZ4 压缩）
- 📦 公开入口 `import jt;`，也可按需 `import jt.log;` / `import jt.detail;`
- ⚡ 零成本抽象

## 技术栈

| 类别 | 技术 |
|------|------|
| **语言** | C++23 (modules, std::format, concepts) |
| **构建系统** | CMake 4.3.0+ (支持 C++23 Modules) |
| **内存管理** | mimalloc - 高性能 allocations |
| **压缩** | lz4 - 快速压缩算法 |
| **网络** | asio - 跨平台异步 I/O |
| **数据格式** | RapidJSON - 高性能 JSON 解析 |

用户入口推荐 `import jt;`（`src/jt.cppm` 再导出 `jt.log` 与 `jt.detail`）。也可以只 `import jt.log;` 或 `import jt.detail;`。

`jt.log.cppm` / `jt.detail.cppm` 只 `export import` 独立命名模块，不再用主模块 `export import` 分区（避免 GCC Darwin 上虚类型 typeinfo 重复）。`logger` / `service` 因循环依赖与 pimpl 留在 `jt.log.core` 分区中；`formatter` / `sink` 及其派生类是独立命名模块。无锁队列、字符串、哈希表等属于 PRIVATE 命名模块，不能通过公开入口使用。

## 项目结构

```
jt/
├── src/
│   ├── jt.cppm                      # 伞模块：export import jt.log / jt.detail
│   ├── jt.log.cppm                  # 日志入口：export import 日志命名模块
│   ├── jt.detail.cppm               # 细节入口：export import 内存/缓冲区命名模块
│   ├── main.cpp                     # 示例程序入口（import jt;）
│   │
│   ├── detail/                      # 底层模块
│   │   ├── config.h                 # 内部：JT_API 可见性宏
│   │   ├── win32.h                  # 内部：Windows / MinGW 头文件包装
│   │   ├── memory.cppm              # 公开命名模块：allocate / unique_ptr
│   │   ├── buffer.cppm              # 公开命名模块：read_buffer / base_memory_buffer
│   │   ├── vector.cppm              # 公开命名模块：vector
│   │   ├── cache_line.cppm          # 内部命名模块：缓存行对齐
│   │   ├── cpu_pause.cppm           # 内部命名模块：CPU 暂停指令
│   │   ├── intrusive_queue.cppm     # 内部命名模块：侵入式单链表队列
│   │   ├── atomic_intrusive_queue.cppm # 内部命名模块：原子侵入式队列
│   │   ├── intrusive_mpsc_queue.cppm  # 内部命名模块：MPSC 队列
│   │   ├── deque.cppm               # 内部命名模块：双端队列
│   │   ├── string.cppm              # 内部命名模块：字符串
│   │   ├── unordered_map.cppm       # 内部命名模块：哈希表
│   │   ├── metric_value.cppm        # 内部命名模块：内存统计
│   │   ├── os.cppm                  # 内部命名模块：操作系统接口
│   │   └── impl/
│   │       ├── buffer.cpp           # module jt.detail.buffer
│   │       ├── memory.cpp           # module jt.detail.memory
│   │       └── os.cpp               # module jt.detail.os
│   │
│   ├── log/                         # 日志系统
│   │   ├── core.cppm                # 公开命名模块 jt.log.core（再导出分区）
│   │   ├── fwd.cppm                 # 公开分区 jt.log.core:fwd
│   │   ├── logger.cppm              # 公开分区 jt.log.core:logger
│   │   ├── service.cppm             # 公开分区 jt.log.core:service
│   │   ├── message.cppm             # 内部实现分区 jt.log.core:message
│   │   ├── level.cppm               # 公开命名模块：日志级别
│   │   ├── record.cppm              # 公开命名模块：log_record_view
│   │   ├── formatter.cppm           # 公开命名模块：格式化器接口
│   │   ├── sink.cppm                # 公开命名模块：日志输出基类
│   │   ├── sink_console.cppm        # 公开命名模块：sink_stdout / sink_stderr
│   │   ├── sink_file.cppm           # 公开命名模块：文件输出（LZ4 压缩）
│   │   ├── functions.cppm           # 公开命名模块：info / warn / error / v*
│   │   ├── default_formatter.cppm   # 内部命名模块：默认格式化器
│   │   └── impl/
│   │       ├── service_impl.cppm    # 内部实现分区 jt.log.core:service_impl
│   │       ├── logger.cpp           # module jt.log.core
│   │       ├── service.cpp          # module jt.log.core
│   │       ├── service_impl.cpp     # module jt.log.core
│   │       ├── formatter.cpp        # module jt.log.formatter
│   │       ├── sink.cpp             # module jt.log.sink
│   │       ├── sink_console.cpp     # module jt.log.sink.console
│   │       └── sink_file.cpp        # module jt.log.sink.file
│   │
│   └── types/                       # 类型定义
│       └── writable_buffer.cppm     # 公开命名模块：writable_buffer 概念
│
├── CMakeLists.txt                   # 构建配置
├── README.md                        # 项目文档
└── AGENTS.md                        # AI 代理开发指南
```

当前构建会生成共享库 `libjt` 和示例程序 `main`。公开模块列在 `JT_PUBLIC_MODULES`，内部模块列在 `JT_PRIVATE_MODULES`（BMI 不向下游传播）。

## 核心功能

### 1. 内存管理

```cpp
import jt;
import std;

void* ptr = jt::detail::allocate(1024);
std::println("allocated size: {}", jt::detail::allocated_size(ptr));
jt::detail::deallocate(ptr);

std::println("total allocated: {}", jt::detail::allocated_memory());
```

- 基于 mimalloc 的高性能内存分配器
- 支持 `allocate` / `deallocate` 接口
- 内存统计: `allocated_memory()`, `allocated_size(void*)`
- 自定义智能指针: `unique_ptr`, `dynamic_unique_ptr`（基类必须有虚析构）

### 2. 缓冲区处理

```cpp
import jt;
import std;

jt::detail::base_memory_buffer<1> buffer;
buffer.append("hello");
std::format_to(std::back_inserter(buffer), " {}", "world");

jt::detail::read_buffer rb(buffer);
std::string_view view(rb);
```

- `read_buffer`: 只读缓冲区，支持零拷贝转换为 `string_view`
- `base_memory_buffer<N>`: 可变长缓冲区，`N` 为栈上内联容量（字节）；不足时改用堆
- 常用别名: `buffer_1k` / `buffer_2k` / `buffer_4k` / `buffer_8k`
- 支持 `std::format` 写入

### 3. 高性能日志系统

```cpp
import jt;
import std;

jt::log::service service;

jt::log::sink_file_config config;
config.daily_rotation = true;
config.directory = "./logs";
config.name = "app";
config.max_size = 1024 * 1024;
config.keep_days = 7;
config.lz4_directory = "./logs/lz4";

std::array sinks{
    jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_file>(
        service, config),
    jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_stdout>()};

const auto log_ptr = service.create_logger(std::move(sinks), "my_logger", true);
auto& log = *log_ptr;

jt::log::info(log, "Hello {}", "World");
jt::log::warn(log, "Memory: {}", jt::detail::allocated_memory());
jt::log::verror(log, "Error: code={}", 500);

service.request_stop();
```

- **生命周期**: 构造 `service` 即启动后台线程；结束时调用 `request_stop()`（析构也会请求停止）
- **创建 logger**: 只能通过 `service::create_logger`；接受可移动的 sink 范围（如 `std::array`），返回 `std::shared_ptr<logger>`。`jt::log::info` 等接口需要 `logger&`
- **多级别**: trace, debug, info, warn, error, critical
- **多输出**: 控制台（`sink_stdout` / `sink_stderr`）、文件（可同时输出到多个目标）
- **文件日志特性**:
  - 按大小轮转（超过 `max_size` 自动切分，默认 200MB）
  - 按日期轮转（`daily_rotation`，默认开启）
  - LZ4 压缩存储（`lz4_directory`，默认保留 `keep_days` 30 天）
- **线程安全**: 异步路径使用内部无锁队列
- **格式化**: 使用 `std::format` 语法；`vinfo` / `vwarn` 等接受运行时格式串（`std::string_view`）

## 构建与运行

### 环境要求

- CMake >= 4.3.0（需要支持 C++23 Modules）
- Clang >= 17 或 GCC >= 13（支持 C++23 模块）
- lz4、asio、RapidJSON、mimalloc 库

### macOS 构建

```bash
# 安装依赖 (使用 Homebrew)
brew install llvm lz4 asio rapidjson mimalloc

# 构建
cmake -B build
cmake --build build

# 运行示例程序
./build/main
```

### Linux 构建

```bash
# 安装依赖 (Ubuntu/Debian)
sudo apt install cmake liblz4-dev libasio-dev rapidjson-dev libmimalloc-dev

# 构建
cmake -B build
cmake --build build

# 运行示例程序
./build/main
```

### Windows 构建

```bash
# 使用 vcpkg 安装依赖
vcpkg install lz4 asio rapidjson mimalloc

# 构建
cmake -B build
cmake --build build

# 运行示例程序
build\main.exe
```

## 使用示例

### 基础日志使用

```cpp
import jt;
import std;

int main() {
  jt::log::service service;

  std::array sinks{
      jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_stdout>()};
  const auto log_ptr =
      service.create_logger(std::move(sinks), "example", true);
  auto& log = *log_ptr;

  jt::log::info(log, "Application started");
  jt::log::warn(log, "This is a warning message");
  jt::log::error(log, "Error occurred: {}", 500);

  service.request_stop();
  return 0;
}
```

按需导入时，把 `import jt;` 换成：

```cpp
import jt.log;
import jt.detail;
```

### 内存管理示例

```cpp
import jt;
import std;

void* ptr = jt::detail::allocate(256);
std::println("Pointer: {}, Size: {}", ptr, jt::detail::allocated_size(ptr));
jt::detail::deallocate(ptr);

std::println("Total memory allocated: {}", jt::detail::allocated_memory());
```

### 缓冲区使用示例

```cpp
import jt;
import std;

jt::detail::base_memory_buffer<1> buffer;
buffer.append("Hello, ");
std::format_to(std::back_inserter(buffer), "World!");

jt::detail::read_buffer rb(buffer);
std::string_view view(rb);
std::println("Buffer content: {}", view);
```

## 平台支持

| 平台 | 架构 | 状态 |
|------|------|------|
| macOS | x64, arm64 | ✅ 完全支持 |
| Linux | x64, arm64 | ✅ 完全支持 |
| Windows | x64 | ✅ 完全支持 |

> **注意**: 当前版本仅支持 64 位系统。

## 开发计划

- [ ] 协程模块 (`coroutine/`)
- [ ] 网络库封装 (基于 asio)
- [ ] 服务器框架核心
- [ ] 单元测试
- [ ] 性能基准测试

## 相关文档

- **AGENTS.md** - AI 代理开发指南，包含代码风格、构建系统和开发规范
- **.clang-format** - 代码格式化配置（Google Style）

## 作者

JT Framework - 现代 C++23 服务器框架实践
