#pragma once

#include <cstdint>

namespace tinylang {

struct FunctionObj;

void profile_on_call(FunctionObj& fn, uint32_t threshold);

}  // namespace tinylang
