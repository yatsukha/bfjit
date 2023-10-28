#include <bfjit/jit/jit.hpp>
#include <bfjit/codegen/instruction_generation_context.hpp>
#include <bfjit/util/terminate.hpp>

#include <bfjit/util/macros.hpp>

BFJIT_WARNINGS_PUSH
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/TargetSelect.h>
BFJIT_WARNINGS_POP

namespace bfjit::jit {

  auto execute(codegen::single_module_context&& smc) -> int32_t {
    // returns true if failed
    bool failed = llvm::InitializeNativeTarget();
    failed = failed || llvm::InitializeNativeTargetAsmParser();
    failed = failed || llvm::InitializeNativeTargetAsmPrinter();
    
    if (failed) {
      util::terminate("Unable to initialize JIT target architecture.");
    }

    auto jit = llvm::cantFail(llvm::orc::LLJITBuilder().create());

    // this one will make sure that symbols in the current process are
    // available to the JIT code
    // essentially to make sure that libc is available, e.g. getchar and putchar
    jit->getMainJITDylib().addGenerator(
      llvm::cantFail(
        llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          jit->getDataLayout().getGlobalPrefix())));

    // fmt::println(
    //   "architecture: {}",
    //   jit->getTargetTriple().getArchName().str());

    llvm::cantFail(
      jit->addIRModule(
        llvm::orc::ThreadSafeModule(
          std::move(smc.mod_impl),
          std::move(smc.ctx_impl))));

    auto main_fn = llvm::cantFail(jit->lookup("main"));
    auto main_ptr = main_fn.toPtr<int32_t(*)()>();

    // for debugging: settings set plugin.jit-loader.gdb.enable on
    return main_ptr();
  }

}
