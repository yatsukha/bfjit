#include <bfjit/optimizer/optimizer.hpp>
#include <bfjit/util/macros.hpp>

BFJIT_WARNINGS_PUSH
#include <llvm/IR/PassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Passes/PassBuilder.h>
BFJIT_WARNINGS_POP

namespace bfjit::optimizer {

  auto pass(codegen::single_module_context const& smc) -> void {
    llvm::LoopAnalysisManager lm;
    llvm::FunctionAnalysisManager fm;
    llvm::CGSCCAnalysisManager cm;
    llvm::ModuleAnalysisManager mm;

    llvm::PassBuilder pb;

    pb.registerModuleAnalyses(mm);
    pb.registerCGSCCAnalyses(cm);
    pb.registerFunctionAnalyses(fm);
    pb.registerLoopAnalyses(lm);
    pb.crossRegisterProxies(lm, fm, cm, mm);

    llvm::ModulePassManager mpm =
      pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

    mpm.run(smc.mod, mm);
  }

}
