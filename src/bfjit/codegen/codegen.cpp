#include <bfjit/codegen/single_module_context.hpp>
#include <bfjit/codegen/codegen.hpp>
#include <bfjit/codegen/instruction_generation_context.hpp>
#include <bfjit/nodes/nodes.hpp>
#include <bfjit/util/macros.hpp>
#include <filesystem>

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
          [&igc](auto&& instr) {
            igc.builder.SetCurrentDebugLocation(igc.dbg.get(instr.loc));
            translate(instr, igc); 
          },
          instruction);
      }
    }

    auto gen_working_memory(
      instruction_generation_context& igc
    ) -> void {
      auto& ctx = igc.smc.ctx;
      auto& builder = igc.builder;

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
    }

    struct entry_type {
      llvm::IRBuilder<> builder;
      debug_context ctx;
    };

    auto gen_entry(
      single_module_context const& global_module,
      debug_metadata const& dbg
    ) -> entry_type {
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
        "jit_main",
        &global_module.mod
      );

      auto* entry_block =
        llvm::BasicBlock::Create(ctx, "entry", main_function);

      auto di_builder = std::make_unique<llvm::DIBuilder>(global_module.mod);
      auto path = std::filesystem::path(dbg.file);
      auto* file =
        di_builder->createFile(
          path.filename().string(), 
          path.parent_path().string());

      auto* compile_unit =
        di_builder->createCompileUnit(
          llvm::dwarf::DW_LANG_C,
          file,
          "BFJIT",
          false,
          "",
          0);

      auto* di_int32 =
        di_builder->createBasicType("int", 32, llvm::dwarf::DW_ATE_signed);
      auto* di_int_ptr_ptr_8 =
        di_builder->createPointerType(
          di_builder->createPointerType(
            di_builder->createBasicType(
              "char",
              8,
              llvm::dwarf::DW_ATE_signed_char),
            64),
          64);

      std::vector<llvm::Metadata*> params = {
        di_int32,
        di_int32,
        di_int_ptr_ptr_8,
      };

      auto* di_func_type =
        di_builder->createSubroutineType(
          di_builder->getOrCreateTypeArray(params));

      auto* di_main_function =
        di_builder->createFunction(
          compile_unit,
          "jit_main",
          "jit_main",
          file,
          0,
          di_func_type,
          0,
          llvm::DINode::FlagPrototyped,
          llvm::DISubprogram::SPFlagDefinition);

      main_function->setSubprogram(di_main_function);

      return {
        llvm::IRBuilder(entry_block),
        debug_context{
          .builder = std::move(di_builder),
          .unit = compile_unit,
          .file = file,
          .main_fn = di_main_function}};
    }

    auto gen_exit(instruction_generation_context& igc) -> void {
      igc.builder.CreateRet(
        llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(igc.smc.ctx), 0));
    }

    auto make_instruction_generation_context(
      single_module_context& smc,
      llvm::IRBuilder<>& builder,
      debug_context&& dbg
    ) -> instruction_generation_context {
      return {
        .smc = smc,
        .mem = {},
        .builder = builder,
        .dbg = std::forward<debug_context>(dbg),
      };
    };

    auto gen_main(
      single_module_context& global_module,
      nodes::instruction_block const& instrs,
      debug_metadata const& dbg
    ) -> void {
      auto [builder, debug_context] = gen_entry(global_module, dbg);
      auto igc = 
        make_instruction_generation_context(
          global_module, builder, std::move(debug_context));

      gen_working_memory(igc);
      gen_instructions(igc, instrs);
      gen_exit(igc);

      igc.dbg.builder->finalize();
    }

  }

  auto gen_ir(
    nodes::instruction_block const& instrs,
    debug_metadata const& dbg
  )
    -> single_module_context {
    auto global_module = single_module_context("global_module");

    gen_main(global_module, instrs, dbg);

    return global_module;
  };

}

