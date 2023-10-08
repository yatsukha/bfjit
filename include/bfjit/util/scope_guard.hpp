#pragma once

#include <memory>

namespace bfjit::util {

  template<typename F>
  [[nodiscard]] auto make_scope_guard(F&& f) noexcept {
    auto deleter = [self = std::forward<F>(f)](auto&&) {
      self();
    };
    return std::unique_ptr<void, decltype(deleter)>{
      reinterpret_cast<void*>(-1),
      std::move(deleter)
    };
  }

}
