#pragma once

#include <memory>

namespace llvm {
  class LLVMContext;
  class Module;
  class Value;
}

namespace bfjit::codegen {

  // keeps a module and its context tied together
  struct single_module_context {
    std::unique_ptr<llvm::LLVMContext> ctx_impl;
    std::unique_ptr<llvm::Module> mod_impl;

    // saving a few keystrokes
    llvm::LLVMContext& ctx = *ctx_impl;
    llvm::Module& mod = *mod_impl;

    single_module_context(char const* module_name);

    single_module_context(single_module_context&&) noexcept = default;

    void output() const;

    ~single_module_context();
  };

}
