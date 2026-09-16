#pragma once

#include <cstdint>
#include <string>

namespace tinylang {

using Value = double;

inline bool value_is_truthy(Value v) { return v != 0.0; }

}  // namespace tinylang
