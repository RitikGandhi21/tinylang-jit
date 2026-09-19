#pragma once

#include <cstdint>

namespace tinylang {

class VM;

extern "C" double tinylang_jit_call1(VM* vm, int fn_index, double arg0);

}  // namespace tinylang
