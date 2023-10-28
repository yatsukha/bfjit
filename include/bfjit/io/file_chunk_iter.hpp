#pragma once

#include <array>
#include <algorithm>
#include <optional>
#include <string>

#include <unistd.h>
#include <fcntl.h>

namespace bfjit::io {

  struct file_chunk_iter {
    int file = -1;
    std::array<char, 4096> buffer;
    ssize_t read_count = 0;

    static auto make(
      std::string const& file
    ) -> std::optional<file_chunk_iter> {
      auto iter = file_chunk_iter{
        file.size()
          ? open(file.c_str(), O_RDONLY)
          : STDIN_FILENO};

      if (iter.file < 0) {
        return {};
      }

      return iter;
    }

    static auto sentinel() -> file_chunk_iter {
      return file_chunk_iter{};
    }

    explicit file_chunk_iter() noexcept = default;

    explicit file_chunk_iter(int const file) noexcept : file(file) {
      if (file < 0) {
        return;
      }

      {
        #ifdef __APPLE__
          int const enable = 1;
          ::fcntl(file, F_RDAHEAD, &enable);
        #elif __linux__
          ::posix_fadvise(file, 0, 0, POSIX_FADV_SEQUENTIAL);
        #endif
      }

      // initialize the iter
      ++(*this);
    }

    file_chunk_iter(file_chunk_iter const&) = delete;
    file_chunk_iter& operator=(file_chunk_iter const&) = delete;

    file_chunk_iter(file_chunk_iter&& other) noexcept {
      *this = std::forward<file_chunk_iter>(other);
    }
    file_chunk_iter& operator=(file_chunk_iter&& other) noexcept {
      file = other.file;
      read_count = other.read_count;
      buffer = std::move(other.buffer);

      // cleanup
      other.file = -1;
      other.read_count = 0;

      return *this;
    }

    ~file_chunk_iter() {
      if (file >= 0) {
        ::close(file);
        file = -1;
      }

      read_count = 0;
    }

    auto operator++() -> file_chunk_iter& {
      read_count =
        ::read(
          file,
          buffer.data(), 
          buffer.size());

      return *this;
    }

    // "deducing this" waiting list
    auto const& operator*() const {
      return *this;
    }
    auto& operator*() {
      return *this;
    }

    auto const* operator->() const {
      return this;
    }
    auto* operator->() {
      return this;
    }

    auto begin() const {
      return buffer.cbegin();
    }

    auto end() const {
      return buffer.cbegin() + std::max(0L, read_count);
    }

    // intended only for sentinel comparison
    friend auto operator==(
      file_chunk_iter const& lhs,
      file_chunk_iter const& rhs
    ) -> bool {
      return lhs.read_count <= 0 && rhs.read_count <= 0;
    }

  };

}
