#include "parser.h"

#include "lexer.h"

#include <stdexcept>

namespace tinylang {

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

const Token& Parser::peek() const { return tokens_.at(current_); }
const Token& Parser::previous() const { return tokens_.at(current_ - 1); }
bool Parser::check(TokenType type) const { return peek().type == type; }
bool Parser::match(TokenType type) {
  if (!check(type)) return false;
  current_++;
  return true;
}

const Token& Parser::consume(TokenType type, const char* message) {
  if (check(type)) return tokens_.at(current_++);
  throw std::runtime_error(message);
}

void Parser::synchronize() {
  current_++;
  while (!check(TokenType::End)) {
    if (previous().type == TokenType::Semicolon) return;
    switch (peek().type) {
      case TokenType::Fn:
      case TokenType::If:
      case TokenType::While:
      case TokenType::Return:
      case TokenType::Print:
      case TokenType::Let:
        return;
      default:
        break;
    }
    current_++;
  }
}

void Parser::emit(Chunk& chunk, Op op, int32_t operand) {
  chunk.code.push_back({op, operand});
}

int Parser::emit_jump(Chunk& chunk, Op op) {
  emit(chunk, op, 0);
  return static_cast<int>(chunk.code.size()) - 1;
}

void Parser::patch_jump(Chunk& chunk, int insn_index, int target_index) {
  chunk.code[insn_index].operand = target_index - insn_index - 1;
}

int Parser::add_constant(Chunk& chunk, double value) {
  chunk.constants.push_back(value);
  return static_cast<int>(chunk.constants.size()) - 1;
}

int Parser::add_local(Chunk& chunk, const std::string& name) {
  locals_.push_back(name);
  local_initialized_.push_back(true);
  chunk.local_count = static_cast<int>(locals_.size());
  return static_cast<int>(locals_.size()) - 1;
}

int Parser::resolve_local(Chunk& chunk, const std::string& name) {
  for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
    if (locals_[i] == name) return i;
  }
  throw std::runtime_error("unknown variable: " + name);
}

Program Parser::parse() {
  Program program;
  fn_map_ = &program.function_index_by_name;
  fn_chunks_ = &program.functions;

  while (match(TokenType::Fn)) {
    Token name = consume(TokenType::Identifier, "expected function name");
    consume(TokenType::LParen, "expected '(' after function name");

    locals_.clear();
    local_initialized_.clear();

    Chunk chunk;
    chunk.name = name.lexeme;

    if (!check(TokenType::RParen)) {
      do {
        Token param = consume(TokenType::Identifier, "expected parameter name");
        add_local(chunk, param.lexeme);
      } while (match(TokenType::Comma));
    }
    chunk.arity = chunk.local_count;
    consume(TokenType::RParen, "expected ')' after parameters");
    consume(TokenType::LBrace, "expected '{' before body");

    int index = static_cast<int>(program.functions.size());
    program.function_index_by_name[chunk.name] = index;
    program.functions.push_back(chunk);
    parse_block(program.functions.back());
  }

  Chunk main_chunk;
  main_chunk.name = "@main";
  locals_.clear();
  local_initialized_.clear();

  while (!check(TokenType::End)) {
    parse_statement(main_chunk);
  }
  emit(main_chunk, Op::OP_RETURN, 0);
  program.functions.push_back(main_chunk);
  program.entry_index = static_cast<int>(program.functions.size()) - 1;
  return program;
}

void Parser::parse_block(Chunk& chunk) {
  while (!check(TokenType::RBrace) && !check(TokenType::End)) {
    parse_statement(chunk);
  }
  consume(TokenType::RBrace, "expected '}'");
}

void Parser::parse_statement(Chunk& chunk) {
  if (match(TokenType::Print)) {
    parse_print(chunk);
    return;
  }
  if (match(TokenType::Let)) {
    parse_var_decl(chunk);
    return;
  }
  if (match(TokenType::If)) {
    parse_if(chunk);
    return;
  }
  if (match(TokenType::While)) {
    parse_while(chunk);
    return;
  }
  if (match(TokenType::Return)) {
    parse_return(chunk);
    return;
  }
  parse_expr_stmt(chunk);
}

void Parser::parse_var_decl(Chunk& chunk) {
  Token name = consume(TokenType::Identifier, "expected variable name");
  consume(TokenType::Assign, "expected '='");
  int slot = add_local(chunk, name.lexeme);
  parse_expression(chunk);
  emit(chunk, Op::OP_SET_LOCAL, slot);
  emit(chunk, Op::OP_POP, 0);
  match(TokenType::Semicolon);
}

void Parser::parse_print(Chunk& chunk) {
  consume(TokenType::LParen, "expected '(' after print");
  parse_expression(chunk);
  consume(TokenType::RParen, "expected ')' after print argument");
  emit(chunk, Op::OP_PRINT, 0);
  match(TokenType::Semicolon);
}

void Parser::parse_if(Chunk& chunk) {
  consume(TokenType::LParen, "expected '(' after if");
  parse_expression(chunk);
  consume(TokenType::RParen, "expected ')' after if condition");
  consume(TokenType::LBrace, "expected '{'");
  int then_jump = emit_jump(chunk, Op::OP_JMP_IF_FALSE);
  emit(chunk, Op::OP_POP, 0);
  parse_block(chunk);
  int else_jump = emit_jump(chunk, Op::OP_JMP);
  patch_jump(chunk, then_jump, static_cast<int>(chunk.code.size()));
  emit(chunk, Op::OP_POP, 0);
  if (match(TokenType::Else)) {
    consume(TokenType::LBrace, "expected '{' after else");
    parse_block(chunk);
  }
  patch_jump(chunk, else_jump, static_cast<int>(chunk.code.size()));
}

void Parser::parse_while(Chunk& chunk) {
  int loop_start = static_cast<int>(chunk.code.size());
  consume(TokenType::LParen, "expected '(' after while");
  parse_expression(chunk);
  consume(TokenType::RParen, "expected ')' after while condition");
  consume(TokenType::LBrace, "expected '{'");
  int exit_jump = emit_jump(chunk, Op::OP_JMP_IF_FALSE);
  emit(chunk, Op::OP_POP, 0);
  parse_block(chunk);
  int back_jump = emit_jump(chunk, Op::OP_JMP);
  patch_jump(chunk, back_jump, loop_start);
  patch_jump(chunk, exit_jump, static_cast<int>(chunk.code.size()));
  emit(chunk, Op::OP_POP, 0);
}

void Parser::parse_return(Chunk& chunk) {
  if (!check(TokenType::Semicolon) && !check(TokenType::RBrace)) {
    parse_expression(chunk);
  } else {
    emit(chunk, Op::OP_PUSH_CONST, add_constant(chunk, 0));
  }
  emit(chunk, Op::OP_RETURN, 0);
  match(TokenType::Semicolon);
}

void Parser::parse_expr_stmt(Chunk& chunk) {
  parse_expression(chunk);
  emit(chunk, Op::OP_POP, 0);
  match(TokenType::Semicolon);
}

void Parser::parse_expression(Chunk& chunk) { parse_assignment(chunk); }
void Parser::parse_assignment(Chunk& chunk) { parse_or(chunk); }
void Parser::parse_or(Chunk& chunk) { parse_and(chunk); }
void Parser::parse_and(Chunk& chunk) { parse_equality(chunk); }

void Parser::parse_equality(Chunk& chunk) {
  parse_comparison(chunk);
  while (match(TokenType::Eq)) {
    parse_comparison(chunk);
    emit(chunk, Op::OP_EQ, 0);
  }
}

void Parser::parse_comparison(Chunk& chunk) {
  parse_term(chunk);
  while (true) {
    if (match(TokenType::Lt)) {
      parse_term(chunk);
      emit(chunk, Op::OP_LT, 0);
    } else if (match(TokenType::Le)) {
      parse_term(chunk);
      emit(chunk, Op::OP_LE, 0);
    } else {
      break;
    }
  }
}

void Parser::parse_term(Chunk& chunk) {
  parse_factor(chunk);
  while (true) {
    if (match(TokenType::Plus)) {
      parse_factor(chunk);
      emit(chunk, Op::OP_ADD, 0);
    } else if (match(TokenType::Minus)) {
      parse_factor(chunk);
      emit(chunk, Op::OP_SUB, 0);
    } else {
      break;
    }
  }
}

void Parser::parse_factor(Chunk& chunk) {
  parse_unary(chunk);
  while (true) {
    if (match(TokenType::Star)) {
      parse_unary(chunk);
      emit(chunk, Op::OP_MUL, 0);
    } else if (match(TokenType::Slash)) {
      parse_unary(chunk);
      emit(chunk, Op::OP_DIV, 0);
    } else {
      break;
    }
  }
}

void Parser::parse_unary(Chunk& chunk) {
  if (match(TokenType::Minus)) {
    parse_unary(chunk);
    emit(chunk, Op::OP_NEG, 0);
    return;
  }
  parse_call(chunk);
}

void Parser::parse_call(Chunk& chunk) {
  if (match(TokenType::Number)) {
    emit(chunk, Op::OP_PUSH_CONST, add_constant(chunk, previous().number));
    return;
  }
  if (match(TokenType::Identifier)) {
    std::string name = previous().lexeme;
    if (match(TokenType::LParen)) {
      int arg_count = 0;
      if (!check(TokenType::RParen)) {
        do {
          parse_expression(chunk);
          arg_count++;
        } while (match(TokenType::Comma));
      }
      consume(TokenType::RParen, "expected ')' after arguments");
      auto it = fn_map_->find(name);
      if (it == fn_map_->end()) {
        throw std::runtime_error("unknown function: " + name);
      }
      if (arg_count != fn_chunks_->at(it->second).arity) {
        throw std::runtime_error("wrong arity for " + name);
      }
      emit(chunk, Op::OP_CALL, it->second);
      return;
    }
    int slot = resolve_local(chunk, name);
    emit(chunk, Op::OP_PUSH_LOCAL, slot);
    return;
  }
  if (match(TokenType::LParen)) {
    parse_expression(chunk);
    consume(TokenType::RParen, "expected ')' after expression");
    return;
  }
  throw std::runtime_error("expected expression");
}

}  // namespace tinylang
