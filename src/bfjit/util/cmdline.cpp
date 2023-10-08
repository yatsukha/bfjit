#include <bfjit/util/cmdline.hpp>

#include <CLI/CLI11.hpp>
#include <fmt/printf.h>

namespace bfjit::util {

  auto parse_opts(int argc, char** argv) -> options {
    options rv;

    CLI::App app{"Brainf💩ck JIT interpreter."};

    app.add_flag(
      "-v,--verbose",
      rv.verbose,
      "Print intermediate results, as well as resulting optimized LLVM IR.");

    app.add_flag(
      "-n,--no-exec",
      rv.noexec,
      "Dont execute, just print the optimized LLVM IR.");

    app.add_option(
      "file.bf",
      rv.input_file,
      "Optional BF file to interpret, otherwise read from stdin until ^D (EOF)."
    )
      ->required(false);

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError const& e) {
      std::exit(app.exit(e));
    }

    return rv;
  }

}
