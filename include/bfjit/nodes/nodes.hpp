#pragma once

#include <bfjit/nodes/location.hpp>

#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include <fmt/ostream.h>

namespace bfjit::codegen {
  struct instruction_generation_context;
}

namespace bfjit::nodes {

  struct token_base {
    location loc;
  };

  struct fwd : token_base {
    friend auto str(fwd const&) noexcept -> std::string {
      return ">";
    }

    friend auto translate(
      fwd const&, codegen::instruction_generation_context&) -> void;
  };

  struct bwd : token_base {
    friend auto str(bwd const&) noexcept -> std::string {
      return "<";
    }

    friend auto translate(
      bwd const&, codegen::instruction_generation_context&) -> void;
  };

  struct inc : token_base {
    friend auto str(inc const&) noexcept -> std::string {
      return "+";
    }

    friend auto translate(
      inc const&, codegen::instruction_generation_context&) -> void;
  };

  struct dec : token_base {
    friend auto str(dec const&) noexcept -> std::string {
      return "-";
    }

    friend auto translate(
      dec const&, codegen::instruction_generation_context&) -> void;
  };

  struct out : token_base {
    friend auto str(out const&) noexcept -> std::string {
      return ".";
    }

    friend auto translate(
      out const&, codegen::instruction_generation_context&) -> void;
  };

  struct inp : token_base {
    friend auto str(inp const&) noexcept -> std::string {
      return ",";
    }

    friend auto translate(
      inp const&, codegen::instruction_generation_context&) -> void;
  };

  struct rep : token_base {
    using variant_instruction = std::variant<
      fwd, bwd,
      inc, dec,
      inp, out,
      rep
    >;

    using instruction_block = std::vector<variant_instruction>;

    location end_location;
    instruction_block instructions;

    friend auto str(rep const& self) noexcept -> std::string {
      std::string rv = "[";
      for (auto&& instr : self.instructions) {
          rv += 
            std::visit(
              [](auto& a) { return str(a); },
              instr);
      }
      rv += "]";
      return rv;
    }

    friend auto translate(
      rep const&, codegen::instruction_generation_context&) -> void;
  };

  using variant_instruction = rep::variant_instruction;
  using instruction_block = rep::instruction_block;

  inline std::ostream& operator<<(
    std::ostream& out, 
    variant_instruction const& v
  ) {
    std::visit(
      [&out](auto&& inner) { out << str(inner); },
      v);
    return out;
  }

}

template<>
struct fmt::formatter<bfjit::nodes::variant_instruction> 
        : ostream_formatter {};
