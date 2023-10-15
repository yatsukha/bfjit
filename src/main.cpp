#include "bfjit/checks/verification.hpp"
#include <bfjit/util/terminate.hpp>
#include <bfjit/util/cmdline.hpp>
#include <bfjit/io/file_chunk_iter.hpp>
#include <bfjit/io/flat_iter.hpp>
#include <bfjit/parser/parser.hpp>
#include <bfjit/codegen/codegen.hpp>
#include <bfjit/optimizer/optimizer.hpp>
#include <bfjit/jit/jit.hpp>

#include <fmt/printf.h>

auto main(int argc, char** argv) -> int {
  auto const args = bfjit::util::parse_opts(argc, argv);
  bool const is_stdin = args.input_file.empty();

  if (is_stdin) {
    fmt::println(
      "STDIN mode, enter a (multiline) program and then emit EOF with ^D.\n");
  }

  auto opt_file_iter =
    bfjit::io::file_chunk_iter::make(args.input_file.c_str());

  if (!opt_file_iter) {
    bfjit::util::terminate(
      "Unable to open file '{}'", args.input_file);
  }

  auto flat_file_iter =
    bfjit::io::flat_iter{
      std::move(*opt_file_iter),
      bfjit::io::file_chunk_iter::sentinel()};
  auto parsed = 
    bfjit::parser::parse(
      std::move(flat_file_iter),
      {.file = args.input_file});

  if (args.verbose) {
    fmt::println("Minified file:");
    for (auto&& top_level_token : parsed) {
      fmt::print("{}", top_level_token);
    }
    fmt::println("\n");
  }

  auto llvm_ir = bfjit::codegen::gen_ir(
    parsed, {.file = is_stdin ? "STDIN" : args.input_file});
  
  if (args.verbose) {
    fmt::println("Before optimizing:\n");
    std::fflush(stdout);
    llvm_ir.output();
  }

  bfjit::checks::check_module(llvm_ir);
  bfjit::optimizer::pass(llvm_ir);

  if (args.verbose || args.noexec) {
    if (args.verbose) {
      fmt::println("\nAfter optimizing:\n");
      std::fflush(stdout);
    }
    llvm_ir.output();
  }

  if (!args.noexec) {
    if (args.verbose || is_stdin) {
      fmt::println("\nRunning\n");
    }
    bfjit::jit::execute(std::move(llvm_ir));
  }
}

