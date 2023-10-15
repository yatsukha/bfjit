#pragma once

#include <bfjit/codegen/single_module_context.hpp>
#include <bfjit/nodes/location.hpp>
#include <bfjit/util/macros.hpp>

BFJIT_WARNINGS_PUSH
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/DIBuilder.h>
BFJIT_WARNINGS_POP

namespace bfjit::codegen {

  struct working_memory {
    enum index : size_t {
      base = 0,
      ptr,
      limit
    };
    // start, current, and one off end
    // these are alloca locations
    // so in essence they are three pointers to a pointer to data
    std::array<llvm::Value*, 3> memory;
  };

  struct debug_context {
    std::unique_ptr<llvm::DIBuilder> builder;
    llvm::DICompileUnit* unit;
    llvm::DIFile* file;
    // set at a later date when main is generated
    // not really satisfied with this design, but that is what happens
    // when you pack these things together into a single context struct
    llvm::DISubprogram* main_fn = nullptr;

    auto get(nodes::location const& loc) -> llvm::DILocation* {
      return llvm::DILocation::get(
        unit->getContext(),
        loc.row, loc.col, 
        main_fn);
    }
  };

  struct instruction_generation_context {
    single_module_context& smc;
    working_memory mem;
    llvm::IRBuilder<>& builder;
    debug_context dbg;

    //
    // utility variables (and variables that are types)
    // 

    // a single point where we can switch the data type to 64 bit
    llvm::IntegerType* data_type = llvm::Type::getInt32Ty(smc.ctx);
    llvm::PointerType* pointer_type =  llvm::Type::getInt64PtrTy(smc.ctx);
    llvm::IntegerType* pointer_data_type = llvm::Type::getInt64Ty(smc.ctx);

    llvm::ConstantInt* alloc_size =
      llvm::ConstantInt::get(
        pointer_data_type,
        smc.mod.getDataLayout().getTypeAllocSize(data_type));
    llvm::ConstantInt* alloc_bit_size =
      llvm::ConstantInt::get(
        pointer_data_type,
        smc.mod.getDataLayout().getTypeAllocSizeInBits(data_type));

    llvm::ConstantInt* zero = llvm::ConstantInt::get(data_type, 0);
    llvm::ConstantInt* one =  llvm::ConstantInt::get(data_type, 1);

    //
    // utility methods for pointer and data management
    //

    template<working_memory::index Idx = working_memory::ptr>
    auto load_ptr() {
      return builder.CreateLoad(pointer_type, mem.memory[Idx]);
    }

    template<working_memory::index Idx = working_memory::ptr>
    auto load_ptr_as_int() {
      return builder.CreatePtrToInt(load_ptr<Idx>(), pointer_data_type);
    }

    template<working_memory::index Idx = working_memory::ptr>
    auto store_ptr(llvm::Value* value) {
      return builder.CreateStore(
        value->getType()->isPointerTy()
          ? value
          : builder.CreateIntToPtr(value, pointer_type),
        mem.memory[Idx]);
    }

    // only makes sense to load/store data at working_memory::ptr
    // and not at base/limit
    auto load() {
      return
        builder.CreateLoad(
          data_type,
          builder.CreateLoad(
            pointer_type,
            mem.memory[working_memory::ptr]));
    }

    auto store(llvm::Value* value) {
      return
        builder.CreateStore(
          value,
          builder.CreateLoad(
            pointer_type,
            mem.memory[working_memory::ptr]));
    }
  };

}

