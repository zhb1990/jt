import std;
import jt.log.core;
import jt.log.functions;
import jt.log.sink.console;
import jt.log.format;
import jt.base.buffer;

#include "format_value.h"

void format_other(jt::base::buffer_1k& output) {
  jt::log::format_to(output, "{} {:04} {:.2f} {:?}", format_value{"other"}, 7,
                     1.25, std::string_view{"a\nb"});
}

void log_other(jt::log::logger& log) {
  jt::log::info(log, "{} {}", format_value{"other"}, 7);
  jt::log::vinfo(log, "{} {}", format_value{"runtime"}, 8);
  jt::log::vinfo(log, "{", 1);  // Formatting failure must be contained.
  jt::log::print_stdout("format consumer {}\n", 2);
  jt::log::print_stderr("format consumer {}\n", format_value{"other"});
}
