#include "vm.h"

#include "jit/code_cache.h"
#include "jit/profile.h"
#include "jit/runtime.h"
#include "jit/template_jit.h"

#include <iostream>
#include <stdexcept>

namespace tinylang {

VM::VM(std::vector<FunctionObj>&& functions, int entry_function_index)
    : functions_(std::move(functions)), entry_function_index_(entry_function_index) {
  for (std::size_t i = 0; i < functions_.size(); ++i) {
    functions_[i].id = static_cast<int>(i);
  }
}

bool VM::maybe_compile(int fn_index) {
#if !defined(__aarch64__)
  return false;
#else
  FunctionObj& fn = functions_.at(fn_index);
  if (fn.native_entry || fn.compiling) return fn.native_entry != nullptr;
  if (fn.invoke_count < jit_threshold_) return false;
  fn.compiling = true;
  void* entry = jit::compile_function(*this, fn);
  fn.compiling = false;
  if (!entry) return false;
  fn.native_entry = entry;
  jit::global_code_cache().finalize();
  return true;
#endif
}

extern "C" double tinylang_jit_call1(VM* vm, int fn_index, double arg0) {
  return vm->call_function(fn_index, {arg0});
}

Value VM::call_function(int fn_index, const std::vector<Value>& args) {
  FunctionObj& fn = functions_.at(fn_index);
  if (args.size() != static_cast<std::size_t>(fn.chunk.arity)) {
    throw std::runtime_error("arity mismatch");
  }

  profile_on_call(fn, jit_threshold_);
  if (jit_enabled_) maybe_compile(fn_index);

#if defined(__aarch64__)
  if (jit_enabled_ && fn.native_entry && fn.chunk.arity == 1) {
    auto native = reinterpret_cast<double (*)(double)>(fn.native_entry);
    return native(args[0]);
  }
#endif

  size_t stack_base = stack_.size();
  for (Value v : args) stack_.push_back(v);
  frames_.push_back({fn_index, 0, stack_base});

  Value result = 0;
  while (!frames_.empty()) {
    Frame& frame = frames_.back();
    FunctionObj& cur = functions_.at(frame.fn_index);
    const Chunk& chunk = cur.chunk;

    if (frame.ip >= chunk.code.size()) {
      throw std::runtime_error("ip out of range in " + chunk.name);
    }

    const Instruction& ins = chunk.code[frame.ip++];
    switch (ins.op) {
      case Op::OP_PUSH_CONST:
        stack_.push_back(chunk.constants.at(ins.operand));
        break;
      case Op::OP_PUSH_LOCAL:
        stack_.push_back(stack_.at(frame.stack_base + ins.operand));
        break;
      case Op::OP_SET_LOCAL:
        stack_.at(frame.stack_base + ins.operand) = stack_.back();
        stack_.pop_back();
        break;
      case Op::OP_POP:
        stack_.pop_back();
        break;
      case Op::OP_ADD: {
        double b = stack_.back();
        stack_.pop_back();
        stack_.back() += b;
        break;
      }
      case Op::OP_SUB: {
        double b = stack_.back();
        stack_.pop_back();
        stack_.back() -= b;
        break;
      }
      case Op::OP_MUL: {
        double b = stack_.back();
        stack_.pop_back();
        stack_.back() *= b;
        break;
      }
      case Op::OP_DIV: {
        double b = stack_.back();
        stack_.pop_back();
        stack_.back() /= b;
        break;
      }
      case Op::OP_NEG:
        stack_.back() = -stack_.back();
        break;
      case Op::OP_LT: {
        double b = stack_.back();
        stack_.pop_back();
        stack_.back() = stack_.back() < b ? 1.0 : 0.0;
        break;
      }
      case Op::OP_LE: {
        double b = stack_.back();
        stack_.pop_back();
        stack_.back() = stack_.back() <= b ? 1.0 : 0.0;
        break;
      }
      case Op::OP_EQ: {
        double b = stack_.back();
        stack_.pop_back();
        stack_.back() = stack_.back() == b ? 1.0 : 0.0;
        break;
      }
      case Op::OP_JMP:
        frame.ip += ins.operand;
        break;
      case Op::OP_JMP_IF_FALSE: {
        if (!value_is_truthy(stack_.back())) frame.ip += ins.operand;
        break;
      }
      case Op::OP_CALL: {
        int callee = ins.operand;
        FunctionObj& target = functions_.at(callee);
        int arity = target.chunk.arity;
        if (stack_.size() < static_cast<size_t>(arity)) {
          throw std::runtime_error("stack underflow on call");
        }
        size_t arg_base = stack_.size() - arity;
        std::vector<Value> call_args(stack_.begin() + arg_base, stack_.end());
        stack_.resize(arg_base);

        profile_on_call(target, jit_threshold_);
        if (jit_enabled_) maybe_compile(callee);

#if defined(__aarch64__)
        if (jit_enabled_ && target.native_entry && target.chunk.arity == 1) {
          auto native = reinterpret_cast<double (*)(double)>(target.native_entry);
          stack_.push_back(native(call_args[0]));
          break;
        }
#endif
        size_t new_base = stack_.size();
        for (Value v : call_args) stack_.push_back(v);
        frames_.push_back({callee, 0, new_base});
        break;
      }
      case Op::OP_RETURN: {
        if (stack_.size() > frame.stack_base) {
          result = stack_.back();
          stack_.pop_back();
        } else {
          result = 0;
        }
        stack_.resize(frame.stack_base);
        frames_.pop_back();
        if (frames_.empty()) return result;
        stack_.push_back(result);
        break;
      }
      case Op::OP_PRINT:
        std::cout << stack_.back() << "\n";
        stack_.pop_back();
        break;
      default:
        throw std::runtime_error("unhandled opcode in interpreter");
    }
  }
  return result;
}

Value VM::run() { return call_function(entry_function_index_, {}); }

}  // namespace tinylang
