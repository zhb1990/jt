# JT 框架说明文档

> C++23 模块化游戏服务器框架 | 基于 mimalloc 的高性能内存管理

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-4.3.0+-blue.svg)](https://cmake.org/)

## 项目概述

JT 是一个现代 C++23 编写的轻量级服务器框架，参考了 skynet 的设计理念，采用 C++23 模块化架构。项目专注于高性能、低延迟的服务器开发，特别适用于游戏服务器后端场景。

**核心特性**：
- 🚀 基于 mimalloc 的高性能内存管理
- 📝 高性能日志系统（异步写入、文件轮转、LZ4 压缩）
- 📦 单模块入口 `import jt;`，内部 partition 不进入公开接口
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

用户入口只有 `import jt;`。公开 interface partition 由 `jt.cppm` 全部 `export import`；内部 implementation partition 使用 `module jt:xxx;`（无 `export`），不会进入公开模块接口。无锁队列、字符串、哈希表等容器属于内部 partition，不能通过 `import jt;` 使用。

## 项目结构

```
jt/
├── src/
│   ├── jt.cppm                      # 主模块导出（仅公开 partition）
│   ├── main.cpp                     # 示例程序入口
│   │
│   ├── detail/                      # 底层模块
│   │   ├── config.h                 # 内部：JT_API 可见性宏
│   │   ├── memory.cppm              # 公开：内存管理 (allocate/deallocate)
│   │   ├── buffer.cppm              # 公开：缓冲区 (read_buffer/base_memory_buffer)
│   │   ├── vector.cppm              # 公开：向量容器
│   │   ├── cache_line.cppm          # 内部：缓存行对齐
│   │   ├── cpu_pause.cppm           # 内部：CPU 暂停指令
│   │   ├── intrusive_queue.cppm     # 内部：侵入式单链表队列
│   │   ├── atomic_intrusive_queue.cppm # 内部：原子侵入式队列
│   │   ├── intrusive_mpsc_queue.cppm  # 内部：MPSC 队列
│   │   ├── deque.cppm               # 内部：双端队列
│   │   ├── string.cppm              # 内部：字符串
│   │   ├── unordered_map.cppm       # 内部：哈希表
│   │   ├── metric_value.cppm        # 内部：内存统计
│   │   ├── os.cppm                  # 内部：操作系统接口
│   │   └── impl/
│   │       ├── buffer.cpp           # 缓冲区实现
│   │       ├── memory.cpp           # 内存管理实现
│   │       └── os.cpp               # OS 实现
│   │
│   ├── log/                         # 日志系统
│   │   ├── level.cppm               # 公开：日志级别
│   │   ├── fwd.cppm                 # 公开：前向声明
│   │   ├── record.cppm              # 公开：log_record_view
│   │   ├── formatter.cppm           # 公开：格式化器接口
│   │   ├── sink.cppm                # 公开：日志输出基类
│   │   ├── logger.cppm              # 公开：日志器
│   │   ├── service.cppm             # 公开：日志服务
│   │   ├── sink_console.cppm        # 公开：控制台输出
│   │   ├── sink_file.cppm           # 公开：文件输出 (LZ4 压缩)
│   │   ├── functions.cppm           # 公开：info/warn/error/critical/v*
│   │   ├── message.cppm             # 内部：异步队列节点
│   │   ├── default_formatter.cppm   # 内部：默认格式化器
│   │   └── impl/
│   │       ├── service_impl.cppm    # 内部：service Pimpl
│   │       ├── logger.cpp           # 日志器实现
│   │       ├── service.cpp          # 日志服务实现
│   │       ├── sink.cpp             # 输出基类实现
│   │       ├── sink_console.cpp     # 控制台输出实现
│   │       ├── sink_file.cpp        # 文件输出实现
│   │       └── service_impl.cpp     # service Pimpl 实现
│   │
│   └── types/                       # 类型定义
│       └── writable_buffer.cppm     # 可写缓冲区概念
│
├── tests/
│   ├── consumer_public.cpp          # 只 import jt; 的独立消费者
│   └── should_fail/hidden.cpp       # 确认内部类型不可见
│
├── CMakeLists.txt                   # 构建配置
├── README.md                        # 项目文档
└── AGENTS.md                        # AI 代理开发指南
```

当前构建会生成共享库 `libjt` 和示例程序 `main`。`tests/` 源码已在仓库中，但尚未接入 `CMakeLists.txt`。

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
- 自定义智能指针: `unique_ptr`, `dynamic_unique_ptr`

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
- `base_memory_buffer<N>`: 可变长缓冲区（模板参数为容量指数）
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
- **创建 logger**: `create_logger` 接受可移动的 sink 范围（如 `std::array`），返回 `std::shared_ptr<logger>`；`jt::log::info` 等接口需要 `logger&`
- **多级别**: trace, debug, info, warn, error, critical
- **多输出**: 控制台（`sink_stdout` / `sink_stderr`）、文件（可同时输出到多个目标）
- **文件日志特性**:
  - 按大小轮转（超过 `max_size` 自动切分）
  - 按日期轮转（每天一个新文件）
  - LZ4 压缩存储（节省磁盘空间）
- **线程安全**: 异步路径使用内部无锁队列
- **格式化**: 使用 `std::format` 语法；`vinfo` / `vwarn` 等接受运行时格式串

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
- [ ] 将 `tests/` 接入 CMake / CTest（public consumer 与内部可见性编译失败检查）
- [ ] 单元测试
- [ ] 性能基准测试

## 相关文档

- **AGENTS.md** - AI 代理开发指南，包含代码风格、构建系统和开发规范
- **.clang-format** - 代码格式化配置（Google Style）

## 作者

JT Framework - 现代 C++23 服务器框架实践
