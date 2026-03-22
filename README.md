# JT 框架说明文档

> C++23 模块化游戏服务器框架 | 基于 mimalloc 的高性能内存管理

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-4.3.0+-blue.svg)](https://cmake.org/)

## 项目概述

JT 是一个现代 C++23 编写的轻量级服务器框架，参考了 skynet 的设计理念，采用 C++23 模块化架构。项目专注于高性能、低延迟的服务器开发，特别适用于游戏服务器后端场景。

**核心特性**：
- 🚀 基于 mimalloc 的高性能内存管理
- 🔒 无锁数据结构支持
- 📝 高性能日志系统（支持 LZ4 压缩）
- 📦 模块化设计，按需引入
- ⚡ 零成本抽象

## 技术栈

| 类别 | 技术 |
|------|------|
| **语言** | C++23 (/modules, std::format, concepts) |
| **构建系统** | CMake 4.3.0+ (支持 C++23 Modules) |
| **内存管理** | mimalloc - 高性能 allocations |
| **压缩** | lz4 - 快速压缩算法 |
| **网络** | asio - 跨平台异步 I/O |
| **数据格式** | RapidJSON - 高性能 JSON 解析 |

## 项目结构

```
jt/
├── src/
│   ├── jt.cppm                      # 主模块导出
│   ├── main.cpp                     # 示例程序入口
│   │
│   ├── coroutine/                   # 协程模块 (待开发)
│   │
│   ├── detail/                      # 底层实现模块
│   │   ├── cache_line.cppm          # 缓存行对齐工具
│   │   ├── cpu_pause.cppm           # CPU 暂停指令
│   │   ├── memory.cppm              # 内存管理 (allocate/deallocate)
│   │   ├── buffer.cppm              # 缓冲区 (read_buffer/write_buffer)
│   │   ├── intrusive_queue.cppm     # 侵入式单链表队列
│   │   ├── atomic_intrusive_queue.cppm # 原子侵入式队列
│   │   ├── intrusive_mpsc_queue.cppm  # MPSC 队列
│   │   ├── deque.cppm               # 双端队列
│   │   ├── vector.cppm              # 向量容器
│   │   ├── string.cppm              # 字符串工具
│   │   ├── unordered_map.cppm       # 哈希表
│   │   ├── metric_value.cppm        # 指标值统计
│   │   ├── os.cppm                  # 操作系统接口
│   │   └── impl/
│   │       ├── buffer.cpp           # 缓冲区实现
│   │       ├── memory.cpp           # 内存管理实现
│   │       └── os.cpp               # OS 实现
│   │
│   ├── log/                         # 日志系统模块
│   │   ├── level.cppm               # 日志级别 (trace/debug/info/warn/error/critical)
│   │   ├── fwd.cppm                 # 前向声明
│   │   ├── logger.cppm              # 日志器实现
│   │   ├── message.cppm             # 日志消息结构
│   │   ├── service.cppm             # 日志服务
│   │   ├── formatter.cppm           # 格式化器接口
│   │   ├── default_formatter.cppm   # 默认格式化器
│   │   ├── sink.cppm                # 日志输出基类
│   │   ├── sink_console.cppm        # 控制台输出
│   │   ├── sink_file.cppm           # 文件输出 (支持 LZ4 压缩)
│   │   ├── functions.cppm           # 日志宏 (info/warn/error/critical/v*)
│   │   └── impl/
│   │       ├── logger.cpp           # 日志器实现
│   │       ├── service.cpp          # 日志服务实现
│   │       ├── sink.cpp             # 输出基类实现
│   │       ├── sink_console.cpp     # 控制台输出实现
│   │       └── sink_file.cpp        # 文件输出实现
│   │
│   └── types/                       # 类型定义
│       └── writable_buffer.cppm     # 可写缓冲区类型
│
├── CMakeLists.txt                   # 构建配置
├── README.md                        # 项目文档
└── AGENTS.md                        # AI 代理开发指南
```

## 核心功能

### 1. 内存管理

```cpp
import jt;

// 使用 mimalloc 进行内存分配
void* ptr = jt::detail::allocate(1024);
jt::log::info(logger, "allocated size: {}", jt::detail::allocated_size(ptr));
jt::detail::deallocate(ptr);

// 内存统计
std::println("total allocated: {}", jt::detail::allocated_memory());
```

- 基于 mimalloc 的高性能内存分配器
- 支持 `allocate` / `deallocate` 接口
- 内存统计: `allocated_memory()`, `allocated_size(void*)`
- 自定义智能指针: `unique_ptr`, `dynamic_unique_ptr`

### 2. 缓冲区处理

```cpp
import jt;

// 可变长缓冲区
jt::detail::base_memory_buffer<1> buffer;
buffer.append("hello");
std::format_to(std::back_inserter(buffer), " {}", "world");

// 只读缓冲区
jt::detail::read_buffer rb(buffer);
std::string_view view(rb);
```

- `read_buffer`: 只读缓冲区，支持零拷贝转换为 `string_view`
- `base_memory_buffer<N>`: 可变长缓冲区（模板参数为容量指数）
- 支持 `std::format` 写入

### 3. 无锁队列

```cpp
import jt;

// 单消费者
jt::detail::intrusive_queue queue;
queue.push(node);
auto node = queue.pop();

// 多生产者单消费者
jt::detail::intrusive_mpsc_queue mpsc;
mpsc.push(node);
```

- `intrusive_queue`: 侵入式单链表队列
- `atomic_intrusive_queue`: 原子侵入式队列（线程安全）
- `intrusive_mpsc_queue`: 多生产者单消费者队列
- 无锁设计，零内存分配

### 4. 高性能日志系统

```cpp
import jt;

// 创建日志服务
jt::log::service service;
service.start();

// 配置文件日志
jt::log::sink_file_config config;
config.daily_rotation = true;     // 按日期轮转
config.directory = "./logs";
config.name = "app";
config.max_size = 1024 * 1024;    // 按大小轮转阈值
config.keep_days = 7;              // 日志保留天数
config.lz4_directory = "./logs/lz4"; // LZ4 压缩文件目录

// 创建日志器
auto sink1 = jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_file>(service, config);
auto sink2 = jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_stdout>();
auto logger = service.create_logger({sink1, sink2}, "my_logger", true);

// 打印日志
jt::log::info(logger, "Hello {}", "World");
jt::log::warn(logger, "Memory: {}", jt::detail::allocated_memory());
jt::log::verror(logger, "Error: code={}", 500);

service.stop();
```

- **多级别**: trace, debug, info, warn, error, critical
- **多输出**: 控制台、文件（可同时输出到多个目标）
- **文件日志特性**:
  - 按大小轮转（超过 `max_size` 自动切分）
  - 按日期轮转（每天一个新文件）
  - LZ4 压缩存储（节省磁盘空间）
- **线程安全**: 无锁设计，支持高并发日志输出
- **格式化**: 使用 `std::format` 语法

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

int main(int argc, char** argv) {
    // 初始化日志服务
    jt::log::service service;
    service.start();

    // 创建控制台输出
    auto sink = jt::detail::make_dynamic_unique<jt::log::sink, jt::log::sink_stdout>();
    auto logger = service.create_logger({sink}, "example", true);

    // 打印日志
    jt::log::info(logger, "Application started");
    jt::log::warn(logger, "This is a warning message");
    jt::log::error(logger, "Error occurred: {}", 500);

    service.stop();
    return 0;
}
```

### 内存管理示例

```cpp
import jt;

void* ptr = jt::detail::allocate(256);
std::println("Pointer: {}, Size: {}", ptr, jt::detail::allocated_size(ptr));
jt::detail::deallocate(ptr);

std::println("Total memory allocated: {}", jt::detail::allocated_memory());
```

### 缓冲区使用示例

```cpp
import jt;

// 创建缓冲区并写入数据
jt::detail::base_memory_buffer<1> buffer;
buffer.append("Hello, ");
std::format_to(std::back_inserter(buffer), "World!");

// 读取缓冲区内容
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
