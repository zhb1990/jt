// Test-only allocation failure after the archive temporary has been flushed.
// Keep system headers and replacement allocation functions out of modules.
#include <sys/stat.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <new>
namespace {
char path[4096]{};
std::atomic_bool enabled{false};
std::atomic_int failed{0};
}  // namespace
void arm_archive_fault(const char* p) {
  std::strncpy(path, p, sizeof(path) - 1);
  enabled = true;
}
int archive_faults() { return failed.load(); }
void* operator new(std::size_t n) {
  if (enabled.load()) {
    struct stat s{};
    if (::stat(path, &s) == 0 && s.st_size > 0 && enabled.exchange(false)) {
      ++failed;
      throw std::bad_alloc();
    }
  }
  if (auto* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
