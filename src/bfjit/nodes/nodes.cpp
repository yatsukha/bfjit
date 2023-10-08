#include <bfjit/nodes/nodes.hpp>
#include <bfjit/codegen/instruction_generation_context.hpp>
#include <bfjit/util/macros.hpp>

BFJIT_WARNINGS_PUSH
#include <llvm/IR/Function.h>
#include <llvm/IR/Constant.h>
BFJIT_WARNINGS_POP

namespace bfjit::nodes {

  auto create_mem_check(codegen::instruction_generation_context& igc) -> void {

    auto static abort_fn = [&] {
      auto static abort_fn_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(igc.smc.ctx), {});

      auto inner = llvm::Function::Create(
        abort_fn_type,
        llvm::Function::ExternalLinkage,
        "abort",
        &igc.smc.mod);

      inner->addFnAttr(llvm::Attribute::NoReturn);
      return inner;
    }();

    auto lt_base = igc.builder.CreateCmp(
      llvm::CmpInst::ICMP_ULT,
      igc.load_ptr_as_int<codegen::working_memory::ptr>(),
      igc.load_ptr_as_int<codegen::working_memory::base>());

    auto gte_limit = igc.builder.CreateCmp(
      llvm::CmpInst::ICMP_UGE,
      igc.load_ptr_as_int<codegen::working_memory::ptr>(),
      igc.load_ptr_as_int<codegen::working_memory::limit>());

    auto short_or = igc.builder.CreateAdd(
      igc.builder.CreateIntCast(lt_base, igc.data_type, false),
      igc.builder.CreateIntCast(gte_limit, igc.data_type, false));

    auto should_abort = igc.builder.CreateCmp(
      llvm::CmpInst::ICMP_UGT,
      short_or,
      igc.zero);

    auto then_block = 
      llvm::BasicBlock::Create(
        igc.smc.ctx, "if", igc.builder.GetInsertBlock()->getParent());
    auto else_block =
      llvm::BasicBlock::Create(
        igc.smc.ctx, "else", igc.builder.GetInsertBlock()->getParent());
    auto merge_block =
      llvm::BasicBlock::Create(
        igc.smc.ctx, "merge", igc.builder.GetInsertBlock()->getParent());

    igc.builder.CreateCondBr(should_abort, then_block, else_block);

    igc.builder.SetInsertPoint(then_block);
    igc.builder.CreateCall(abort_fn);
    igc.builder.CreateBr(merge_block);

    igc.builder.SetInsertPoint(else_block);
    igc.builder.CreateBr(merge_block);

    igc.builder.SetInsertPoint(merge_block);
  }

  auto translate(
    fwd const&, codegen::instruction_generation_context& igc
  ) -> void {
    igc.store_ptr(
      igc.builder.CreateAdd(
        igc.load_ptr_as_int(),
        igc.builder.CreateIntCast(
          igc.alloc_bit_size,
          igc.pointer_data_type,
          false),
        "",
        true,
        true));

    create_mem_check(igc);
  }

  auto translate(
    bwd const&, codegen::instruction_generation_context& igc
  ) -> void {
    igc.store_ptr(
      igc.builder.CreateSub(
        igc.load_ptr_as_int(),
        igc.builder.CreateIntCast(
          igc.alloc_bit_size,
          igc.pointer_data_type,
          false),
        "",
        true,
        true));

    create_mem_check(igc);
  }

  auto translate(
    inc const&, codegen::instruction_generation_context& igc
  ) -> void {
    igc.store(
      igc.builder.CreateAdd(
        igc.load(),
        igc.one));
  }

  auto translate(
    dec const&, codegen::instruction_generation_context& igc
  ) -> void {
    igc.store(
      igc.builder.CreateSub(
        igc.load(),
        igc.one));
  }

  auto translate(
    out const&, codegen::instruction_generation_context& igc
  ) -> void {
    auto static* putchar_fn_type = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(igc.smc.ctx),
      { llvm::Type::getInt32Ty(igc.smc.ctx) },
      false);

    auto static* putchar_fn = llvm::Function::Create(
      putchar_fn_type,
      llvm::Function::ExternalLinkage,
      "putchar",
      &igc.smc.mod);

    // this is essentially a noop since we are using 32 bit data type
    auto* arg = igc.builder.CreateIntCast(
      igc.load(),
      llvm::Type::getInt32Ty(igc.smc.ctx),
      true);

    igc.builder.CreateCall(
      putchar_fn,
      { arg });
  }

  auto translate(
    inp const&, codegen::instruction_generation_context& igc
  ) -> void {
    auto static* getchar_fn_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(igc.smc.ctx),
        {},
        false);

    auto static* getchar_fn = llvm::Function::Create(
      getchar_fn_type,
      llvm::Function::ExternalLinkage,
      "getchar",
      &igc.smc.mod); 
    
    auto* call_instr = igc.builder.CreateCall(getchar_fn, {});

    auto* is_eof = igc.builder.CreateICmpEQ(
      call_instr,
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(igc.smc.ctx), -1, true));

    auto then_block = 
      llvm::BasicBlock::Create(
        igc.smc.ctx, "if", igc.builder.GetInsertBlock()->getParent());
    auto else_block =
      llvm::BasicBlock::Create(
        igc.smc.ctx, "else", igc.builder.GetInsertBlock()->getParent());
    auto merge_block =
      llvm::BasicBlock::Create(
        igc.smc.ctx, "merge", igc.builder.GetInsertBlock()->getParent());

    igc.builder.CreateCondBr(is_eof, then_block, else_block);

    // set to zero to enable breaking out of input loops on EOF
    igc.builder.SetInsertPoint(then_block);
    igc.store(igc.zero);
    igc.builder.CreateBr(merge_block);
  
    // set to actual value
    igc.builder.SetInsertPoint(else_block);
    igc.store(
      igc.builder.CreateIntCast(
          call_instr,
          igc.data_type,
          true));
    igc.builder.CreateBr(merge_block);

    // continuation point
    igc.builder.SetInsertPoint(merge_block);
  }

  auto translate(
    rep const& self, codegen::instruction_generation_context& igc
  ) -> void {
    auto const make_non_zero_check = [&igc]{
      return igc.builder.CreateICmpNE(
        igc.load(),
        igc.zero);
    };

    auto loop_block = 
      llvm::BasicBlock::Create(
        igc.smc.ctx, "loop", igc.builder.GetInsertBlock()->getParent());

    auto exit_block = 
      llvm::BasicBlock::Create(
        igc.smc.ctx, "exit", igc.builder.GetInsertBlock()->getParent());

    auto* is_non_zero = make_non_zero_check();

    (void) igc.builder.CreateCondBr(is_non_zero, loop_block, exit_block);

    igc.builder.SetInsertPoint(loop_block);
    for (auto const& instruction : self.instructions) {
      std::visit(
        [&igc](auto&& instr) { translate(instr, igc); },
        instruction);
    }
    
    auto* continue_looping = make_non_zero_check();
    (void) igc.builder.CreateCondBr(continue_looping, loop_block, exit_block);

    igc.builder.SetInsertPoint(exit_block);
  }

}
