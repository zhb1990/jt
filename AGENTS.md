# AGENTS.md - JT Framework Codebase Guide

## Build System

**Build Commands:**
- `cmake -B build` - Configure build
- `cmake --build build` - Build project
- `cmake --build build --target main` - Build executable
- `rm -rf build && cmake -B build` - Clean rebuild

**Build Requirements:**
- CMake >= 4.1.2 (required for C++23 modules support)
- C++23 compiler (Clang >= 17 or GCC >= 13)
- Libraries: lz4, asio, RapidJSON, mimalloc

**Platform Support:**
- macOS (x64, arm64)
- Linux (x64, arm64)
- Windows (x64 only)

**Note:** 32-bit architectures are explicitly rejected in CMakeLists.txt

## Testing

**Current State:** No automated test suite configured.

**Manual Testing:**
- Run `./build/main` to execute the sample application
- This tests all core modules: memory management, logging, buffer handling

**Adding Tests:**
- Create test files in `tests/` directory
- Link against `libjt` shared library
- Use standard C++ test frameworks (Google Test, Catch2, etc.)

## Code Style Guidelines

### File Organization
- **Module files:** `.cppm` extension for C++23 modules
- **Implementation files:** `.cpp` extension
- **Header files:** `.h` for internal config headers
- Module hierarchy: `jt:detail.*`, `jt:log.*`

### Naming Conventions
- **Namespaces:** `jt::detail` for internals, `jt::log` for public API
- **Classes:** PascalCase (e.g., `logger`, `service`, `buffer`)
- **Methods:** PascalCase (e.g., `should_log`, `create_logger`)
- **Variables:** snake_case (e.g., `max_size`, `keep_days`)
- **Type aliases:** snake_case (e.g., `sink_ptr`)

### C++23 Module Guidelines
- Use `export module jt:module.name;` syntax
- Export entire namespaces: `export namespace jt::log { ... }`
- Internal modules use `module;` preamble, exported modules use `export module;`
- Import dependencies before exports: `import std;` first

### Formatting
- Follow Google style (configured in `.clang-format`)
- Use 2 spaces for indentation
- Line length: ~120 characters
- Braces on same line for functions/classes
- Include `<detail/config.h>` first in module files with `#pragma once`

### Error Handling
- Use exceptions for recoverable errors in logging
- Noexcept for destructors and critical low-level operations
- Try-catch blocks in log functions to prevent logging failures from cascading
- Return empty/invalid state on error rather than throwing in destructors

### Memory Management
- Use custom allocator-based containers from `jt::detail`
- `allocator<T>` for all standard library containers
- Custom smart pointers: `unique_ptr`, `dynamic_unique_ptr`
- Memory statistics via `allocated_memory()`, `allocated_size()`

### Logging API
- **Formatted logging:** `jt::log::info(logger, "msg {}", arg);`
- **Variable argument logging:** `jt::log::vinfo(logger, "msg", fmt, args...);`
- **Log levels:** trace, debug, info, warn, error, critical
- **Structured logging:** Use source_location for file/line info
- Thread-safe: All log functions are thread-safe

### Import Conventions
- Module imports: `import jt;` or `import jt:log.logger;`
- Standard library: `import std;`
- Internal module dependencies: `import :detail.buffer;`

### Type Definitions
- Use `std::uint32_t`, `std::int64_t` for explicit-width integers
- `std::string_view` for read-only string parameters
- `std::shared_ptr` for shared ownership (e.g., logger)
- `detail::dynamic_unique_ptr` for polymorphic unique pointers
- `std::format_string<Args...>` for format string type safety

### Module Structure
- **Detail module files** (internal implementation): `src/detail/*.cppm`
- **Impl files** (compiled implementations): `src/detail/impl/*.cpp`
- **Log module files:** `src/log/*.cppm`
- **Log impl files:** `src/log/impl/*.cpp`

## Cursor/Copilot Rules

**No specific rules file found.** Project relies on:
- `.clang-format` with `BasedOnStyle: Google`
- C++23 module best practices
- Project-specific conventions documented above

## Quick Start for AI Agents

1. **Understanding the codebase:** Focus on `jt.cppm` as the main module export point
2. **Modifying logging:** Edit files in `src/log/` directory
3. **Adding data structures:** Add to `src/detail/` with corresponding `.cppm` files
4. **Testing changes:** Run `./build/main` to verify no regressions
5. **Code style:** Follow existing patterns, use `clang-format` to verify

## Project Goals

- High-performance server framework with focus on low latency
- C++23 modules for fast compilation and clear dependencies
- Memory efficiency via mimalloc integration
- Zero-cost abstractions for production use
