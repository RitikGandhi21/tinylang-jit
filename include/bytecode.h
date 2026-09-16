#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tinylang {

enum class Op : uint8_t {
  OP_PUSH_CONST,
  OP_PUSH_LOCAL,
  OP_SET_LOCAL,
  OP_POP,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_LT,
  OP_LE,
  OP_EQ,
  OP_NEG,
  OP_JMP,
  OP_JMP_IF_FALSE,
  OP_CALL,
  OP_RETURN,
  OP_PRINT,
};

struct Instruction {
  Op op;
  int32_t operand = 0;
};

struct Chunk {
  std::vector<Instruction> code;
  std::vector<double> constants;
  int arity = 0;
  int local_count = 0;
  std::string name;
};

const char* op_name(Op op);

}  // namespace tinylang
