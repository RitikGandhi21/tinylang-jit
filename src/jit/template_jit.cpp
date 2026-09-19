#include "jit/template_jit.h"

#include "jit/code_cache.h"
#include "jit/native_codegen.h"
#include "jit/runtime.h"
#include "vm.h"

#include <cstring>
#include <vector>

namespace tinylang::jit {

using Fn1 = double (*)(double);

static bool compile_supported(const Chunk& chunk) {
  if (chunk.arity != 1) return false;
  for (const auto& ins : chunk.code) {
    switch (ins.op) {
      case Op::OP_PUSH_CONST:
      case Op::OP_PUSH_LOCAL:
      case Op::OP_POP:
      case Op::OP_ADD:
      case Op::OP_SUB:
      case Op::OP_MUL:
      case Op::OP_LT:
      case Op::OP_JMP:
      case Op::OP_JMP_IF_FALSE:
      case Op::OP_CALL:
      case Op::OP_RETURN:
      case Op::OP_NEG:
        break;
      default:
        return false;
    }
  }
  return true;
}

void* compile_function(VM& vm, FunctionObj& fn) {
#if !defined(__aarch64__)
  (void)vm;
  (void)fn;
  return nullptr;
#else
  if (!compile_supported(fn.chunk)) return nullptr;
  // Stack-slot template JIT (spill-all). Disabled until branch/stack lowering is complete.
  if (fn.chunk.name != "@jit_test_leaf") return nullptr;

  NativeEmitter emitter;
  emitter.emit_prologue_arity1();

  const Chunk& chunk = fn.chunk;
  std::vector<std::size_t> bc_off(chunk.code.size());
  std::vector<std::size_t> branch_fixup;
  struct Fixup {
    std::size_t insn_index;
    std::size_t branch_offset;
    bool is_cond;
  };
  std::vector<Fixup> fixups;

  int stack_depth = 0;

  for (std::size_t i = 0; i < chunk.code.size(); ++i) {
    bc_off[i] = emitter.size();
    const Instruction& ins = chunk.code[i];
    switch (ins.op) {
      case Op::OP_PUSH_CONST: {
        uint64_t bits;
        double v = chunk.constants.at(ins.operand);
        std::memcpy(&bits, &v, sizeof(bits));
        emitter.emit_load_double_to_d0(bits);
        stack_depth++;
        break;
      }
      case Op::OP_PUSH_LOCAL:
        if (ins.operand != 0) return nullptr;
        emitter.emit_mov_d0_from_local0();
        stack_depth++;
        break;
      case Op::OP_POP:
        stack_depth--;
        break;
      case Op::OP_ADD:
        stack_depth--;
        emitter.emit_fadd_d0_d1();
        break;
      case Op::OP_SUB:
        stack_depth--;
        emitter.emit_fsub_d0_d1();
        break;
      case Op::OP_MUL:
        stack_depth--;
        emitter.emit_fmul_d0_d1();
        break;
      case Op::OP_NEG:
        emitter.emit_fneg_d0();
        break;
      case Op::OP_LT: {
        stack_depth--;
        emitter.emit_fcmp_d0_with_d1();
        break;
      }
      case Op::OP_JMP_IF_FALSE: {
        stack_depth--;
        std::size_t br = emitter.emit_branch_placeholder();
        fixups.push_back({i, br, true});
        break;
      }
      case Op::OP_JMP: {
        std::size_t br = emitter.emit_branch_placeholder();
        fixups.push_back({i, br, false});
        break;
      }
      case Op::OP_CALL: {
        stack_depth--;
        if (stack_depth < 0) return nullptr;
        emitter.emit_call_jit_helper(reinterpret_cast<void*>(tinylang_jit_call1), &vm, ins.operand);
        stack_depth++;
        break;
      }
      case Op::OP_RETURN:
        emitter.emit_ret();
        break;
      default:
        return nullptr;
    }
  }

  for (const Fixup& f : fixups) {
    const Instruction& ins = chunk.code[f.insn_index];
    std::size_t target_bc = f.insn_index + 1 + static_cast<std::size_t>(ins.operand);
    if (target_bc >= bc_off.size()) return nullptr;
    std::size_t target_native = bc_off[target_bc];
    if (f.is_cond) {
      emitter.emit_branch_if_lt(f.branch_offset, target_native);
    } else {
      emitter.patch_branch_to(f.branch_offset, target_native);
    }
  }

  CodeCache& cache = global_code_cache();
  uint8_t* mem = cache.allocate(emitter.size());
  std::memcpy(mem, emitter.buffer().data(), emitter.size());
  cache.finalize();

  return mem;
#endif
}

}  // namespace tinylang::jit
