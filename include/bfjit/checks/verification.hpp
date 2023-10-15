#pragma once

#include <bfjit/codegen/single_module_context.hpp>

namespace bfjit::checks {
  
  // prints and exits if errors
  auto check_module(codegen::single_module_context const& smc) -> void;

}
