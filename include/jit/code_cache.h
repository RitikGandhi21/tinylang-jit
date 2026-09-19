#pragma once

#include <cstddef>
#include <cstdint>

namespace tinylang::jit {

class CodeCache {
 public:
  CodeCache();
  ~CodeCache();

  CodeCache(const CodeCache&) = delete;
  CodeCache& operator=(const CodeCache&) = delete;

  // Allocate `size` bytes in the executable region. Returns writable pointer.
  uint8_t* allocate(std::size_t size);

  void finalize();  // flip mapping to RX (W^X)

 private:
  uint8_t* base_ = nullptr;
  std::size_t capacity_ = 0;
  std::size_t used_ = 0;
  bool finalized_ = false;
};

CodeCache& global_code_cache();

}  // namespace tinylang::jit
