#include "jit/profile.h"

#include "vm.h"

namespace tinylang {

void profile_on_call(FunctionObj& fn, uint32_t threshold) {
  fn.invoke_count++;
  (void)threshold;
}

}  // namespace tinylang
