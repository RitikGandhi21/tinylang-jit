#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tinylang {

enum class TokenType {
  End,
  Identifier,
  Number,
  Fn,
  If,
  Else,
  While,
  Return,
  Print,
  Let,
  LParen,
  RParen,
  LBrace,
  RBrace,
  Comma,
  Semicolon,
  Plus,
  Minus,
  Star,
  Slash,
  Lt,
  Le,
  Eq,
  Assign,
};

struct Token {
  TokenType type;
  std::string lexeme;
  double number = 0;
  int line = 1;
};

class Lexer {
 public:
  explicit Lexer(std::string_view source);
  std::vector<Token> tokenize();

 private:
  std::string_view source_;
  size_t pos_ = 0;
  int line_ = 1;

  bool at_end() const;
  char peek() const;
  char advance();
  void skip_whitespace();
  Token make_identifier();
  Token make_number();
};

}  // namespace tinylang
