#pragma once

#include <string>

namespace bfjit::nodes {

  struct location {
    std::string file;
    size_t row = 0;
    size_t col = 0;
  };

}
