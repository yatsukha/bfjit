#pragma once

#include <bfjit/nodes/nodes.hpp>
#include <bfjit/util/scope_guard.hpp>
#include <bfjit/util/terminate.hpp>

#include <optional>
#include <vector>

namespace bfjit::parser {

  template<typename SelfContainedIter>
  auto parse_once(
    SelfContainedIter& iter
  ) -> std::optional<nodes::rep::variant_instruction> {

    if (!iter) {
      return {};
    }

    auto defer_increment = bfjit::util::make_scope_guard([&iter]{
      ++iter;
    });

    switch (*iter) {
      case '>':
        return nodes::fwd{};
      case '<':
        return nodes::bwd{};
      case '+':
        return nodes::inc{};
      case '-':
        return nodes::dec{};
      case '.':
        return nodes::out{};
      case ',':
        return nodes::inp{};
      case '[': {
        ++iter;
        auto rep = nodes::rep{};

        while (iter && *iter != ']') {
          if (auto opt_instr = parse_once(iter)) {
            rep.instructions.emplace_back(std::move(*opt_instr));
          }
        }

        if (!iter) {
          util::terminate("Expected ']', got EOF");
        }

        return rep;
      };
      case ']':
        util::terminate("Unexpected ']'");
      default:
        // ignore unknown chars
        return {};
    }
  }

  template<typename SelfContainedIter>
  auto parse(SelfContainedIter&& iter) -> decltype(nodes::rep::instructions) {
    decltype(nodes::rep::instructions) instructions;

    while (iter) {
      if (auto opt_instr = parse_once(iter)) {
        instructions.emplace_back(std::move(*opt_instr));
      }
    }

    return instructions;
  }

}

