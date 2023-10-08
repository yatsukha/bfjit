#pragma once

#include <iterator>

#include <fmt/printf.h>

namespace bfjit::io {

  namespace detail {

    struct sentinel_type {
      friend auto operator<=>(sentinel_type const&, sentinel_type const&)
        = default;
    };

  }

  template<typename Iterator, typename Sentinel>
  struct flat_iter;

  template<>
  struct flat_iter<detail::sentinel_type, detail::sentinel_type> {};

  template<typename Iterator, typename Sentinel>
  struct flat_iter {
    Iterator begin;
    Sentinel end;

    decltype(std::begin(*begin)) inner{std::begin(*begin)};

    decltype(auto) operator*() const {
      return *inner;
    }

    auto* operator->() const {
      return &(*inner);
    }
    
    operator bool() const {
      return begin != end;
    }

    auto& operator++() {
      if (!*this) {
        return *this;
      }

      ++inner;

      if (inner == std::end(*begin)) {
        ++begin;

        if (!*this) {
          return *this;
        }

        inner = std::begin(*begin);
      }

      return *this;
    }

    auto operator++(int) {
      auto copy = *this;
      ++(*this);
      return copy;
    }

    template<typename Iter, typename Sent>
    friend bool operator==(
      flat_iter<Iter, Sent> const& lhs,
      flat_iter<detail::sentinel_type, detail::sentinel_type> const&
    ) {
      return !static_cast<bool>(lhs);
    }

    template<typename I1, typename I2, typename S1, typename S2>
    friend bool operator==(
      flat_iter<I1, S1> const& lhs,
      flat_iter<I2, S2> const& rhs
    ) {
      return lhs.begin == rhs.begin && lhs.inner == rhs.inner;
    }
  };

  template<typename Iterator, typename Sentinel>
  flat_iter(Iterator&&, Sentinel&&) -> flat_iter<Iterator, Sentinel>;

  flat_iter() -> flat_iter<detail::sentinel_type, detail::sentinel_type>;

}

