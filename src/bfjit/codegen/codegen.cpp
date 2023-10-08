#include <bfjit/codegen/codegen.hpp>
#include <bfjit/codegen/instruction_generation_context.hpp>
#include <bfjit/nodes/nodes.hpp>
#include <bfjit/util/macros.hpp>

BFJIT_WARNINGS_PUSH
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constant.h>
BFJIT_WARNINGS_POP

namespace bfjit::codegen {

  namespace {

    auto gen_instructions(
      instruction_generation_context& igc,
      nodes::instruction_block const& instrs
    ) -> void {
      for (auto const& instruction : instrs) {
        std::visit(
          [&igc](auto&& instr) { translate(instr, igc); },
          instruction);
      }
    }

    auto gen_working_memory(
      single_module_context& global_module,
      llvm::IRBuilder<>& builder
    ) -> instruction_generation_context {
      auto& ctx = global_module.ctx;

      auto igc = instruction_generation_context{
        .smc = global_module,
        .mem = {},
        .builder = builder,
      };

      igc.mem = working_memory{.memory = {
        builder.CreateAlloca(igc.pointer_type, nullptr, "memory_base"),
        builder.CreateAlloca(igc.pointer_type, nullptr, "memory_ptr"),
        builder.CreateAlloca(igc.pointer_type, nullptr, "memory_limit"),
      }};

      // size of malloc
      auto* malloc_size =
        llvm::ConstantInt::get(igc.pointer_data_type, 1 << 28);

      // create a instruction reserving signed memory for our cells
      auto* malloc_call = builder.Insert(
        llvm::CallInst::CreateMalloc(
          builder.GetInsertBlock(),
          // size type for target architecture
          igc.pointer_data_type,
          // type of memory we are allocating
          igc.data_type,
          // size of the type
          igc.alloc_size,
          // number of elements
          malloc_size,
          nullptr,
          "memory_base"));

      igc.store_ptr<working_memory::base>(malloc_call);
      igc.store_ptr<working_memory::ptr>(malloc_call);

      auto* memory_size_in_bits = builder.CreateMul(
        malloc_size,
        igc.alloc_bit_size);

      (void) builder.CreateMemSet(
        malloc_call,
        llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx), 0),
        // can be also statically calculated tbh
        builder.CreateUDiv(
          memory_size_in_bits,
          llvm::ConstantInt::get(igc.pointer_data_type, 8)),
        llvm::MaybeAlign(1));

      igc.store_ptr<working_memory::limit>(
        builder.CreateAdd(
          igc.load_ptr_as_int<working_memory::base>(),
          memory_size_in_bits,
          "",
          true, // no overflow (only a 1 in (1^64 - malloc size) chance :^))
          true));

      return igc;
    }

    auto gen_entry(
      single_module_context const& global_module
    ) -> llvm::IRBuilder<> {
      auto& ctx = global_module.ctx;

      auto* main_type = llvm::FunctionType::get(
        // return value
        llvm::Type::getInt32Ty(ctx),
        // argument types
        {
          // argc
          llvm::Type::getInt32Ty(ctx),
          // argv, pointer to bytes
          llvm::Type::getInt8PtrTy(ctx)->getPointerTo(),
        },
        // is not variable argument function
        false
      );

      auto* main_function = llvm::Function::Create(
        // type of the function
        main_type,
        // external linkage per convention
        llvm::Function::ExternalLinkage,
        "main",
        &global_module.mod
      );

      auto* entry_block =
        llvm::BasicBlock::Create(ctx, "entry", main_function);

      return llvm::IRBuilder(entry_block);
    }

    auto gen_exit(instruction_generation_context& igc) -> void {
      igc.builder.CreateRet(
        llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(igc.smc.ctx), 0));
    }

    auto gen_main(
      single_module_context& global_module,
      nodes::instruction_block const& instrs
    ) -> void {
      auto builder = gen_entry(global_module);
      auto igc = gen_working_memory(global_module, builder);

      gen_instructions(igc, instrs);
      gen_exit(igc);
    }

  }

  auto gen_ir(
    nodes::instruction_block const& instrs
  )
    -> single_module_context {
    auto global_module = single_module_context("global_module");

    gen_main(global_module, instrs);

    return global_module;
  };

}

