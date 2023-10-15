#include <bfjit/checks/verification.hpp>
#include <bfjit/util/terminate.hpp>
#include <bfjit/util/macros.hpp>

BFJIT_WARNINGS_PUSH
#include <llvm/IR/Verifier.h>
BFJIT_WARNINGS_POP

namespace bfjit::checks {
  
  // prints and exits if errors
  auto check_module(codegen::single_module_context const& smc) -> void {
    bool broken_debug = false;
    if (
      llvm::verifyModule(smc.mod, &llvm::errs(), &broken_debug)
      || broken_debug
    ) {
      util::terminate("Failed to verify module and/or debug information");
    }
  }

}
