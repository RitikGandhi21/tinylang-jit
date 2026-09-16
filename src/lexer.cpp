#include "lexer.h"

#include <cctype>
#include <stdexcept>

namespace tinylang {

Lexer::Lexer(std::string_view source) : source_(source) {}

bool Lexer::at_end() const { return pos_ >= source_.size(); }

char Lexer::peek() const {
  if (at_end()) return '\0';
  return source_[pos_];
}

char Lexer::advance() {
  char c = peek();
  if (c == '\n') line_++;
  if (!at_end()) pos_++;
  return c;
}

void Lexer::skip_whitespace() {
  while (!at_end()) {
    char c = peek();
    if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
      advance();
      continue;
    }
    if (c == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
      while (!at_end() && peek() != '\n') advance();
      continue;
    }
    break;
  }
}

Token Lexer::make_identifier() {
  size_t start = pos_;
  while (!at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
    advance();
  }
  std::string text(source_.substr(start, pos_ - start));
  TokenType type = TokenType::Identifier;
  if (text == "fn") type = TokenType::Fn;
  else if (text == "if") type = TokenType::If;
  else if (text == "else") type = TokenType::Else;
  else if (text == "while") type = TokenType::While;
  else if (text == "return") type = TokenType::Return;
  else if (text == "print") type = TokenType::Print;
  else if (text == "let") type = TokenType::Let;
  return Token{type, text, 0, line_};
}

Token Lexer::make_number() {
  size_t start = pos_;
  while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
  if (peek() == '.') {
    advance();
    while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
  }
  std::string text(source_.substr(start, pos_ - start));
  return Token{TokenType::Number, text, std::stod(text), line_};
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  while (!at_end()) {
    skip_whitespace();
    if (at_end()) break;
    char c = peek();
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      tokens.push_back(make_identifier());
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
      tokens.push_back(make_number());
      continue;
    }
    int line = line_;
    switch (c) {
      case '(':
        advance();
        tokens.push_back({TokenType::LParen, "(", 0, line});
        break;
      case ')':
        advance();
        tokens.push_back({TokenType::RParen, ")", 0, line});
        break;
      case '{':
        advance();
        tokens.push_back({TokenType::LBrace, "{", 0, line});
        break;
      case '}':
        advance();
        tokens.push_back({TokenType::RBrace, "}", 0, line});
        break;
      case ',':
        advance();
        tokens.push_back({TokenType::Comma, ",", 0, line});
        break;
      case ';':
        advance();
        tokens.push_back({TokenType::Semicolon, ";", 0, line});
        break;
      case '+':
        advance();
        tokens.push_back({TokenType::Plus, "+", 0, line});
        break;
      case '-':
        advance();
        tokens.push_back({TokenType::Minus, "-", 0, line});
        break;
      case '*':
        advance();
        tokens.push_back({TokenType::Star, "*", 0, line});
        break;
      case '/':
        advance();
        tokens.push_back({TokenType::Slash, "/", 0, line});
        break;
      case '<':
        advance();
        if (peek() == '=') {
          advance();
          tokens.push_back({TokenType::Le, "<=", 0, line});
        } else {
          tokens.push_back({TokenType::Lt, "<", 0, line});
        }
        break;
      case '=':
        advance();
        if (peek() == '=') {
          advance();
          tokens.push_back({TokenType::Eq, "==", 0, line});
        } else {
          tokens.push_back({TokenType::Assign, "=", 0, line});
        }
        break;
      default:
        throw std::runtime_error(std::string("unexpected char '") + c + "' at line " + std::to_string(line));
    }
  }
  tokens.push_back({TokenType::End, "", 0, line_});
  return tokens;
}

}  // namespace tinylang
