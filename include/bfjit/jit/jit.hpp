#pragma once

#include <bfjit/codegen/single_module_context.hpp>

#include <cstdint>

namespace bfjit::jit {

  auto execute(codegen::single_module_context&& smc) -> int32_t;

}
