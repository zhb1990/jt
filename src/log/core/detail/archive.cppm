module;

#include <lz4frame.h>

module jt.log.core:archive;

import std;
import jt.base.containers;

namespace jt::log {

struct lz4_data {
  lz4_data();
  ~lz4_data() noexcept;
  void compress(const base::string& src, const base::string& directory);
  bool compress_file(std::ofstream& output, std::uint64_t& count_out,
                     std::uint64_t& count_in, std::ifstream& input);
  base::vector<char> input_chunk;
  base::vector<char> output_buff;
  LZ4F_compressionContext_t ctx{nullptr};
  bool ctx_invalid{false};
};

class archive_worker {
 public:
  archive_worker() = default;
  ~archive_worker() noexcept;
  void start();
  void request_stop();
  void wait_stop();
  void post_lz4(const std::filesystem::path& file_name,
                std::string_view directory);
  void clear_lz4(const base::string& name, std::string_view directory,
                 std::uint32_t keep_days);

 private:
  struct lz4_message {
    enum class type { lz4, clear };
    type tp{type::lz4};
    base::string lz4_directory;
    base::string file_name;
    std::uint32_t keep_days{0};
  };

  void push_lz4_message(lz4_message& msg);
  void clear_lz4_files(const lz4_message& msg);
  void lz4_run();

  std::thread lz4_thread_{};
  base::deque<lz4_message> lz4_queue_{};
  std::mutex lz4_mutex_{};
  // All waits use unique_lock<mutex>; avoid condition_variable_any's internal
  // make_shared<mutex> and its additional libstdc++ control-block instantiation.
  std::condition_variable lz4_cv_{};
  lz4_data lz4_data_;
  bool lz4_stop_requested_{false};
};

}  // namespace jt::log
