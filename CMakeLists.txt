cmake_minimum_required(VERSION 3.10)

project(bfjit)

# C++23 triggers a bug in llvm16, fixed in llvm17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS_DEBUG 
  "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE 
  "${CMAKE_CXX_FLAGS_RELEASE} -g -march=native")


# -DCUSTOM_CXX_LINKER="--ld-path=/usr/local/bin/ld64.sold"
# otherwise it uses the compiler to deduce it
if(DEFINED CUSTOM_CXX_LINKER)
  add_link_options(${CUSTOM_CXX_LINKER})
endif()

add_executable(bfjit 
  src/main.cpp
  src/bfjit/nodes/nodes.cpp
  src/bfjit/codegen/single_module_context.cpp
  src/bfjit/codegen/codegen.cpp
  src/bfjit/checks/verification.cpp
  src/bfjit/optimizer/optimizer.cpp
  src/bfjit/jit/jit.cpp
  src/bfjit/util/cmdline.cpp)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
# -DLLVM_DIR=/opt/homebrew/opt/llvm/lib/cmake/llvm
find_package(LLVM REQUIRED)

set(llvm_libs LLVM)

if(APPLE)
  llvm_map_components_to_libnames(llvm_libs 
    core
    analysis
    scalaropts
    instcombine
    transformutils
    passes
    orcjit
    native)
endif()

message(STATUS "LLVM found at: ${LLVM_DIR}")
message(STATUS "LLVM lib list: ${llvm_libs}")

target_link_libraries(bfjit PRIVATE Threads::Threads fmt::fmt ${llvm_libs})

target_include_directories(bfjit PRIVATE 
  "include"
  ${LLVM_INCLUDE_DIRS})

