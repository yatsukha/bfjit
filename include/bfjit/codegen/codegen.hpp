#pragma once

#include <bfjit/codegen/single_module_context.hpp>
#include <bfjit/nodes/nodes.hpp>

#include <string>

namespace bfjit::codegen {

  struct debug_metadata {
    std::string file;
  };

  auto gen_ir(
    nodes::instruction_block const& instructions,
    debug_metadata const& dbg
  )
    -> single_module_context;

}
