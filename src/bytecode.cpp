#include "bytecode.h"

namespace tinylang {

const char* op_name(Op op) {
  switch (op) {
    case Op::OP_PUSH_CONST: return "PUSH_CONST";
    case Op::OP_PUSH_LOCAL: return "PUSH_LOCAL";
    case Op::OP_SET_LOCAL: return "SET_LOCAL";
    case Op::OP_POP: return "POP";
    case Op::OP_ADD: return "ADD";
    case Op::OP_SUB: return "SUB";
    case Op::OP_MUL: return "MUL";
    case Op::OP_DIV: return "DIV";
    case Op::OP_LT: return "LT";
    case Op::OP_LE: return "LE";
    case Op::OP_EQ: return "EQ";
    case Op::OP_NEG: return "NEG";
    case Op::OP_JMP: return "JMP";
    case Op::OP_JMP_IF_FALSE: return "JMP_IF_FALSE";
    case Op::OP_CALL: return "CALL";
    case Op::OP_RETURN: return "RETURN";
    case Op::OP_PRINT: return "PRINT";
  }
  return "?";
}

}  // namespace tinylang
