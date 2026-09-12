#include <lz4frame.h>

import std;
import jt;

void arm_archive_fault(const char* path);
int archive_faults();

void check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  namespace fs = std::filesystem;
  const auto directory =
      fs::temp_directory_path() /
      std::format("jt-archive-failure-{}",
                  std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(directory);
  struct cleanup {
    const fs::path& path;
    ~cleanup() {
      std::error_code ec;
      fs::remove_all(path, ec);
    }
  } guard{directory};
  const auto source = directory / "source.log";
  const auto temporary = directory / "source.log.lz4.tmp";
  const auto final = directory / "source.log.lz4";
  const std::string payload = "archive allocation failure recovery";
  {
    std::ofstream file(source);
    file << payload;
  }
  {
    jt::log::service service;
    arm_archive_fault(temporary.c_str());
    service.make_lz4_client().post(source, directory.string());
  }
  check(archive_faults() == 1, "allocation failure was injected");
  check(fs::exists(source), "failed archive preserves source");
  check(!fs::exists(temporary) && !fs::exists(final),
        "exception removes owned temporary without publishing");
  {
    jt::log::service service;
    service.make_lz4_client().post(source, directory.string());
  }
  check(!fs::exists(source) && !fs::exists(temporary) && fs::exists(final),
        "retry publishes archive and cleans up source");
  std::ifstream file(final, std::ios::binary);
  const std::string input{std::istreambuf_iterator<char>(file), {}};
  LZ4F_decompressionContext_t context = nullptr;
  check(!LZ4F_isError(LZ4F_createDecompressionContext(&context, LZ4F_VERSION)),
        "create decompression context");
  char output[128]{};
  auto input_size = input.size();
  auto output_size = sizeof(output);
  const auto remaining = LZ4F_decompress(context, output, &output_size,
                                         input.data(), &input_size, nullptr);
  LZ4F_freeDecompressionContext(context);
  check(remaining == 0 && input_size == input.size() &&
            std::string_view(output, output_size) == payload,
        "retry archive contains the original bytes");
}
