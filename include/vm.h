#pragma once

#include "bytecode.h"
#include "value.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace tinylang {

struct FunctionObj {
  Chunk chunk;
  int id = 0;
  uint64_t invoke_count = 0;
  void* native_entry = nullptr;  // JIT code, nullptr if not compiled
  bool compiling = false;
};

class VM {
 public:
  explicit VM(std::vector<FunctionObj>&& functions, int entry_function_index);

  void set_jit_enabled(bool enabled) { jit_enabled_ = enabled; }
  void set_jit_threshold(uint32_t t) { jit_threshold_ = t; }
  bool jit_enabled() const { return jit_enabled_; }

  Value run();
  Value call_function(int fn_index, const std::vector<Value>& args);

  const std::vector<FunctionObj>& functions() const { return functions_; }
  FunctionObj& function(int index) { return functions_.at(index); }

 private:
  std::vector<FunctionObj> functions_;
  int entry_function_index_;

  std::vector<Value> stack_;
  struct Frame {
    int fn_index;
    size_t ip;
    size_t stack_base;
  };
  std::vector<Frame> frames_;

  bool jit_enabled_ = true;
  uint32_t jit_threshold_ = 32;

  Value run_interpreter();
  bool maybe_compile(int fn_index);
};

}  // namespace tinylang
