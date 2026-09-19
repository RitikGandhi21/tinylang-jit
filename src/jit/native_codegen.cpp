#include "jit/native_codegen.h"

#include <cstring>

namespace tinylang::jit {

void NativeEmitter::reset() { bytes_.clear(); }

void NativeEmitter::emit_u8(uint8_t b) { bytes_.push_back(b); }

void NativeEmitter::emit_u32(uint32_t word) {
  emit_u8(static_cast<uint8_t>(word & 0xff));
  emit_u8(static_cast<uint8_t>((word >> 8) & 0xff));
  emit_u8(static_cast<uint8_t>((word >> 16) & 0xff));
  emit_u8(static_cast<uint8_t>((word >> 24) & 0xff));
}

void NativeEmitter::emit_literal_u64(uint64_t v) {
  for (int i = 0; i < 8; ++i) emit_u8(static_cast<uint8_t>((v >> (i * 8)) & 0xff));
}

#if defined(__aarch64__)

void NativeEmitter::emit_prologue_arity1() {
  emit_u32(0xa9bf7bfd);  // stp x29, x30, [sp, #-16]!
  emit_u32(0x910003fd);  // mov x29, sp
  emit_u32(0x1e624100);  // fmov d8, d0
}

void NativeEmitter::emit_epilogue_arity1() {
  emit_u32(0xa8c17bfd);
  emit_u32(0xd65f03c0);
}

void NativeEmitter::emit_ret() { emit_epilogue_arity1(); }

void NativeEmitter::emit_load_double_to_d0(uint64_t ieee_bits) {
  emit_u32(0x58000040);
  emit_u32(0x14000003);
  emit_literal_u64(ieee_bits);
  emit_u32(0x1e604000);
}

void NativeEmitter::emit_mov_d0_from_local0() { emit_u32(0x1e624100); }
void NativeEmitter::emit_mov_d1_from_local0() { emit_u32(0x1e624101); }
void NativeEmitter::emit_fadd_d0_d1() { emit_u32(0x1e602800); }
void NativeEmitter::emit_fsub_d0_d1() { emit_u32(0x1e603800); }
void NativeEmitter::emit_fmul_d0_d1() { emit_u32(0x1e600800); }
void NativeEmitter::emit_fdiv_d0_d1() { emit_u32(0x1e601800); }
void NativeEmitter::emit_fneg_d0() { emit_u32(0x1e614000); }
void NativeEmitter::emit_fcmp_d0_with_d1() { emit_u32(0x1e602100); }

std::size_t NativeEmitter::emit_branch_placeholder() {
  std::size_t at = size();
  emit_u32(0x14000000);
  return at;
}

void NativeEmitter::patch_branch_to(std::size_t branch_insn_offset, std::size_t target_offset) {
  int32_t imm26 = static_cast<int32_t>((target_offset - branch_insn_offset) / 4);
  uint32_t insn = 0x14000000u | (static_cast<uint32_t>(imm26) & 0x03ffffffu);
  std::memcpy(bytes_.data() + branch_insn_offset, &insn, 4);
}

void NativeEmitter::emit_branch_if_lt(std::size_t branch_insn_offset, std::size_t target_offset) {
  int32_t imm19 = static_cast<int32_t>((target_offset - branch_insn_offset) / 4);
  uint32_t insn = 0x5400000au | ((static_cast<uint32_t>(imm19) & 0x7ffffu) << 5);
  std::memcpy(bytes_.data() + branch_insn_offset, &insn, 4);
}

void NativeEmitter::emit_load_x0(uint64_t ptr) {
  emit_u32(0x58000040);
  emit_u32(0x14000002);
  emit_literal_u64(ptr);
}

void NativeEmitter::emit_load_x16(uint64_t ptr) {
  emit_u32(0x58000050);
  emit_u32(0x14000002);
  emit_literal_u64(ptr);
}

void NativeEmitter::emit_mov_w1(int fn_index) {
  uint32_t imm = static_cast<uint32_t>(fn_index) & 0xffffu;
  emit_u32(0x52800001u | (imm << 5));
}

void NativeEmitter::emit_call_jit_helper(void* helper, void* vm, int fn_index) {
  emit_u32(0xf81f0fe8);  // str d8, [sp, #-16]!

  emit_load_x0(reinterpret_cast<uint64_t>(vm));
  emit_mov_w1(fn_index);
  // d0 already holds argument

  emit_load_x16(reinterpret_cast<uint64_t>(helper));
  emit_u32(0xd63f0200);  // blr x16

  emit_u32(0xf84107e8);  // ldr d8, [sp], #16
}

#else

void NativeEmitter::emit_prologue_arity1() {}
void NativeEmitter::emit_epilogue_arity1() {}
void NativeEmitter::emit_ret() {}
void NativeEmitter::emit_load_double_to_d0(uint64_t) {}
void NativeEmitter::emit_mov_d0_from_local0() {}
void NativeEmitter::emit_mov_d1_from_local0() {}
void NativeEmitter::emit_fadd_d0_d1() {}
void NativeEmitter::emit_fsub_d0_d1() {}
void NativeEmitter::emit_fmul_d0_d1() {}
void NativeEmitter::emit_fdiv_d0_d1() {}
void NativeEmitter::emit_fneg_d0() {}
void NativeEmitter::emit_fcmp_d0_with_d1() {}
std::size_t NativeEmitter::emit_branch_placeholder() { return 0; }
void NativeEmitter::patch_branch_to(std::size_t, std::size_t) {}
void NativeEmitter::emit_branch_if_lt(std::size_t, std::size_t) {}
void NativeEmitter::emit_call_jit_helper(void*, void*, int) {}

#endif

}  // namespace tinylang::jit
