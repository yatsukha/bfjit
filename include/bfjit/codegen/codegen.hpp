#pragma once

#include <bfjit/codegen/single_module_context.hpp>
#include <bfjit/nodes/nodes.hpp>

namespace bfjit::codegen {

  auto gen_ir(
    nodes::instruction_block const&
  )
    -> single_module_context;

}
