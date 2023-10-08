#include <bfjit/codegen/single_module_context.hpp>

#include <bfjit/util/macros.hpp>

BFJIT_WARNINGS_PUSH
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
BFJIT_WARNINGS_POP

namespace bfjit::codegen {

  single_module_context::single_module_context(char const* module_name)
    : ctx_impl(std::make_unique<llvm::LLVMContext>())
    , mod_impl(std::make_unique<llvm::Module>(module_name, *ctx_impl))
  {}

  single_module_context::~single_module_context() = default;

  void single_module_context::output() const {
    mod.print(llvm::outs(), nullptr);
  }

}
