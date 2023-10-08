#pragma once

#include <string>

namespace bfjit::util {

  struct options {
    bool verbose = false;
    bool noexec = false;
    std::string input_file;
  };

  auto parse_opts(int argc, char** argv) -> options;

}
