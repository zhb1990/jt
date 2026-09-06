# AGENTS.md - JT Framework Codebase Guide

## Build System

**Build Commands:**
- `cmake -B build` - Configure build
- `cmake --build build` - Build project
- `cmake --build build --target main` - Build executable
- `rm -rf build && cmake -B build` - Clean rebuild

**Build Requirements:**
- CMake >= 4.3.0 (required for C++23 modules support)
- C++23 compiler (Clang >= 17 or GCC >= 13)
- Libraries: lz4, asio, RapidJSON, mimalloc

**Platform Support:**
- macOS (x64, arm64)
- Linux (x64, arm64)
- Windows (x64 only)

**Note:** 32-bit architectures are explicitly rejected in CMakeLists.txt

**Shared library notes:**
- Target is `libjt` (`PREFIX ""`)
- Windows/Cygwin: compile with `JT_DLL_EXPORT` so `JT_API` is `__declspec(dllexport)` (BMI bakes this in; consumers must not redefine it)
- POSIX: `JT_LIB_VISIBILITY` + `CXX_VISIBILITY_PRESET hidden`; public module TUs also get `-fvisibility=default` so GCC module initializers (`_ZGIW*`) are exported
- MinGW GCC: `--allow-multiple-definition`, link `stdc++exp`, copy runtime DLLs next to `main`

## Code Style Guidelines

### File Organization
- **Module files:** `.cppm` extension for C++23 modules
- **Implementation files:** `.cpp` extension
- **Header files:** `.h` for internal config headers (`config.h`, `win32.h`)
- Module hierarchy: independent named modules `jt.detail.*`, `jt.log.*`; `jt.log.core` still uses partitions

### Naming Conventions
- **Namespaces:** `jt::detail` for internals, `jt::log` for the logging API, `jt::types` for concepts
- **Classes / structs / enums:** snake_case (e.g., `logger`, `service`, `sink_file`, `base_memory_buffer`)
- **Methods / functions:** snake_case (e.g., `should_log`, `create_logger`)
- **Variables:** snake_case (e.g., `max_size`, `keep_days`)
- **Type aliases:** snake_case (e.g., `sink_ptr`, `buffer_1k`)

### C++23 Module Guidelines
- Users may `import jt;` (`src/jt.cppm` re-exports `jt.log` and `jt.detail`). Fine-grained: `import jt.log;` / `import jt.detail;`
- Public APIs are independent named modules (`export module jt.log.sink;`, `export module jt.detail.memory;`)
- `src/jt.log.cppm` / `src/jt.detail.cppm` only `export import` other named modules; they must not `export import :partition`
- `logger` / `service` / `fwd` stay partitions of `jt.log.core` (import cycle + pimpl). Polymorphic types (`formatter`, `sink`, derived sinks) are separate named modules so GCC Darwin will not duplicate their typeinfo
- Internal modules used across named modules are `export module jt.detail.os;` (etc.) in the PRIVATE file set; do not put them in PUBLIC
- `jt.log.core` implementation partitions `:message` and `:service_impl` are in the PRIVATE file set (`module jt.log.core:message;`, not `export module`)
- Implementation units belong to the module that declared the type (`module jt.log.sink;`, `module jt.log.core;`, not a shared `module jt;`)
- Public interface units must not import PRIVATE modules
- Import dependencies before exports: `import std;` first
- Interface units that need `JT_API`: start with a global module fragment (`module;` then `#include` `detail/config.h`, then `export module ...`)

### Formatting
- Follow Google style (configured in `.clang-format`)
- Use 2 spaces for indentation
- Line length: ~120 characters
- Braces on same line for functions/classes

### Error Handling
- Use exceptions for recoverable errors in logging
- Noexcept for destructors and critical low-level operations
- Try-catch blocks in log functions to prevent logging failures from cascading
- Return empty/invalid state on error rather than throwing in destructors

### Memory Management
- Use custom allocator-based containers from `jt::detail`
- `allocator<T>` for all standard library containers
- Custom smart pointers: `unique_ptr`, `dynamic_unique_ptr` (`dynamic_deleter` requires a virtual destructor on the base)
- Memory statistics via `allocated_memory()`, `allocated_size()`

### Logging API
- Construct `jt::log::service` to start backend threads; call `request_stop()` when finished (destructor also requests stop)
- Loggers are created only via `service::create_logger` (`logger::ctor_key` is not user-constructible)
- `create_logger` takes a movable sink range (e.g. `std::array`) and returns `std::shared_ptr<logger>`
- Pass `logger&` (dereference the shared_ptr) to log helpers: `jt::log::info(log, "msg {}", arg);`
- **Variable argument logging:** `jt::log::vinfo(log, fmt, args...);` (`fmt` is `std::string_view`)
- **Log levels:** trace, debug, info, warn, error, critical
- **Structured logging:** Use source_location for file/line info
- Thread-safe: All log functions are thread-safe
- `sink_file` holds a `service::lz4_client` (weak handle); it becomes a no-op after `service` is destroyed

### Import Conventions
- User code: `import jt;` or `import jt.log;` / `import jt.detail;`
- Standard library: `import std;`
- Other named modules: `import jt.log.sink;` / `import jt.detail.os;`
- `jt.log.core` partitions only: `import :fwd;` / `import :logger;` / `import :service;` / `import :message;` / `import :service_impl;`
- PRIVATE named modules (`jt.detail.os`, `jt.log.default_formatter`, queues, string, …) may only be imported by implementation units and other internal modules

### Type Definitions
- Use `std::uint32_t`, `std::int64_t` for explicit-width integers
- `std::string_view` for read-only string parameters
- `std::shared_ptr` for shared ownership (e.g., logger)
- `detail::dynamic_unique_ptr` for polymorphic unique pointers
- `std::format_string<Args...>` for format string type safety

### Module Structure
- **Umbrella:** `src/jt.cppm` (`export module jt;`) re-exports `jt.log` and `jt.detail`
- **Public named modules:** listed in `JT_PUBLIC_MODULES`. Barrels `src/jt.log.cppm` and `src/jt.detail.cppm` re-export them
- **`jt.log.core` public partitions** (PUBLIC file set): `:fwd`, `:logger`, `:service` — exported by `src/log/core.cppm`
- **`jt.log.core` implementation partitions** (PRIVATE file set): `:message`, `:service_impl`
- **Internal named modules:** remaining files in `JT_PRIVATE_MODULES`; `export module` so other library units can import them, PRIVATE so BMIs are not propagated
- **Impl files:** `src/detail/impl/*.cpp`, `src/log/impl/*.cpp` use the matching named module (`module jt.log.sink;`, `module jt.log.core;`, `module jt.detail.buffer;`, …)

## Cursor/Copilot Rules

**No specific rules file found.** Project relies on:
- `.clang-format` with `BasedOnStyle: Google`
- C++23 module best practices
- Project-specific conventions documented above

## Quick Start for AI Agents

1. **Understanding the codebase:** `src/jt.cppm` is the umbrella; `src/jt.log.cppm` and `src/jt.detail.cppm` are the domain barrels
2. **Modifying logging:** Edit files in `src/log/` directory
3. **Adding data structures:** Add to `src/detail/` with corresponding `.cppm` files; put public APIs in `JT_PUBLIC_MODULES` and internals in `JT_PRIVATE_MODULES`
4. **Testing changes:** Run `./build/main` to verify no regressions
5. **Code style:** Follow existing patterns, use `clang-format` to verify

## Project Goals

- High-performance server framework with focus on low latency
- C++23 modules for fast compilation and clear dependencies
- Memory efficiency via mimalloc integration
- Zero-cost abstractions for production use
