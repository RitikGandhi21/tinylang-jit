#pragma once

#include "bytecode.h"
#include "lexer.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace tinylang {

struct Program {
  std::vector<Chunk> functions;
  std::unordered_map<std::string, int> function_index_by_name;
  int entry_index = -1;  // top-level script chunk index, or -1 if only fns + trailing call
};

class Parser {
 public:
  explicit Parser(const std::vector<struct Token>& tokens);
  Program parse();

 private:
  const std::vector<struct Token>& tokens_;
  size_t current_ = 0;

  const struct Token& peek() const;
  const struct Token& previous() const;
  bool check(TokenType type) const;
  bool match(TokenType type);
  const struct Token& consume(TokenType type, const char* message);
  void synchronize();

  void parse_block(Chunk& chunk);
  void parse_statement(Chunk& chunk);
  void parse_var_decl(Chunk& chunk);
  void parse_print(Chunk& chunk);
  void parse_if(Chunk& chunk);
  void parse_while(Chunk& chunk);
  void parse_return(Chunk& chunk);
  void parse_expr_stmt(Chunk& chunk);
  void parse_expression(Chunk& chunk);
  void parse_assignment(Chunk& chunk);
  void parse_or(Chunk& chunk);
  void parse_and(Chunk& chunk);
  void parse_equality(Chunk& chunk);
  void parse_comparison(Chunk& chunk);
  void parse_term(Chunk& chunk);
  void parse_factor(Chunk& chunk);
  void parse_unary(Chunk& chunk);
  void parse_call(Chunk& chunk);

  void emit(Chunk& chunk, Op op, int32_t operand = 0);
  int emit_jump(Chunk& chunk, Op op);
  void patch_jump(Chunk& chunk, int insn_index, int target_index);
  int add_constant(Chunk& chunk, double value);
  int resolve_local(Chunk& chunk, const std::string& name);
  int add_local(Chunk& chunk, const std::string& name);

  std::vector<std::string> locals_;
  std::vector<bool> local_initialized_;

  std::unordered_map<std::string, int>* fn_map_ = nullptr;
  std::vector<Chunk>* fn_chunks_ = nullptr;
};

}  // namespace tinylang
