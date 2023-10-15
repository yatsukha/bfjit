#pragma once

#include <bfjit/nodes/nodes.hpp>
#include <bfjit/util/scope_guard.hpp>
#include <bfjit/util/terminate.hpp>

#include <optional>
#include <vector>

namespace bfjit::parser {

  template<typename SelfContainedIter>
  auto parse_once(
    SelfContainedIter& iter,
    nodes::location& loc
  ) -> std::optional<nodes::rep::variant_instruction> {

    if (!iter) {
      return {};
    }

    auto defer_increment = bfjit::util::make_scope_guard([&iter]{
      ++iter;
    });

    auto defer_loc_move = bfjit::util::make_scope_guard([&loc]{
      ++loc.col;
    });

    switch (*iter) {
      case '>':
        return nodes::fwd{{.loc = loc}};
      case '<':
        return nodes::bwd{{.loc = loc}};
      case '+':
        return nodes::inc{{.loc = loc}};
      case '-':
        return nodes::dec{{.loc = loc}};
      case '.':
        return nodes::out{{.loc = loc}};
      case ',':
        return nodes::inp{{.loc = loc}};
      case '[': {
        ++iter;
        auto rep = nodes::rep{{.loc = loc}, {}, {}};
        ++loc.col;

        while (iter && *iter != ']') {
          if (auto opt_instr = parse_once(iter, loc)) {
            rep.instructions.emplace_back(std::move(*opt_instr));
          }
        }

        if (!iter) {
          util::terminate("Expected ']', got EOF");
        }

        rep.end_location = loc;

        return rep;
      };
      case ']':
        util::terminate(
          "Unexpected ']' at line {}, column {} of {}",
          loc.row + 1,
          loc.col + 1,
          loc.file.size() ? loc.file : "STDIN");

      case '\n':
        defer_loc_move.release();
        ++loc.row;
        loc.col = 0;
        // fallthrough
      default:
        // ignore unknown chars
        return {};
    }
  }

  template<typename SelfContainedIter>
  auto parse(
    SelfContainedIter&& iter,
    nodes::location&& loc
  ) -> decltype(nodes::rep::instructions) {
    decltype(nodes::rep::instructions) instructions;

    while (iter) {
      if (auto opt_instr = parse_once(iter, loc)) {
        instructions.emplace_back(std::move(*opt_instr));
      }
    }

    return instructions;
  }

}

