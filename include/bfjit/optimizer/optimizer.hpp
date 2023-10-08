#pragma once

#include <bfjit/codegen/single_module_context.hpp>

namespace bfjit::optimizer {
  
  auto pass(codegen::single_module_context const& smc) -> void;

}
