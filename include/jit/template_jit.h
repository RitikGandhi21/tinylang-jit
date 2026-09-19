#pragma once

namespace tinylang {

class VM;
struct FunctionObj;

namespace jit {

// Compile fn to native code; returns entry pointer or nullptr on failure.
void* compile_function(VM& vm, FunctionObj& fn);

}  // namespace jit
}  // namespace tinylang
