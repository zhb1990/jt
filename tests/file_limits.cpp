// Keep POSIX headers out of the import-std consumer translation unit.
#include <signal.h>
#include <sys/resource.h>

namespace {
rlimit previous_limit{};
struct sigaction previous_signal{};
bool active = false;
}  // namespace

bool limit_file_size(unsigned long long bytes) {
  if (active || getrlimit(RLIMIT_FSIZE, &previous_limit) != 0) return false;
  struct sigaction ignored{};
  ignored.sa_handler = SIG_IGN;
  sigemptyset(&ignored.sa_mask);
  if (sigaction(SIGXFSZ, &ignored, &previous_signal) != 0) return false;
  auto limit = previous_limit;
  limit.rlim_cur = static_cast<rlim_t>(bytes);
  if (setrlimit(RLIMIT_FSIZE, &limit) != 0) {
    sigaction(SIGXFSZ, &previous_signal, nullptr);
    return false;
  }
  active = true;
  return true;
}

void restore_file_size_limit() noexcept {
  if (!active) return;
  setrlimit(RLIMIT_FSIZE, &previous_limit);
  sigaction(SIGXFSZ, &previous_signal, nullptr);
  active = false;
}
