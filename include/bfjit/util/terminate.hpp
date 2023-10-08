#pragma once

#include <cstdio>
#include <cstdlib>

#include <fmt/printf.h>

namespace bfjit::util {

  template<typename... Args>
  [[noreturn]] auto terminate(
    fmt::format_string<Args...> fmt,
    Args&& ...args
  ) -> void {
    fmt::println(stderr, fmt, std::forward<Args>(args)...);
    std::exit(EXIT_FAILURE);
  }

}
