#pragma once
#include <iostream>
#include <optional>
#include <string.h>
#include <string>
#include <vector>

enum class TokenType {
  return_,
  print,
  while_,
  for_,
  else_,
  int_lit,
  int_,
  str_,
  bool_,
  float_,
  arr_,
  if_,
  elif_,
  _if,
  class_,
  var,
  str_lit,
  char_,
  char_lit,
  curl_open,
  curl_close,
  round_open,
  round_close,
  square_open,
  square_close,
  colon,
  semi,
  comma,
  equal,
  assign,
  not_,
  not_eq_,
  dot,
  plus,
  plus_eq,
  minus,
  minus_eq,
  slash,
  slash_eq,
  star,
  star_eq,
  great,
  great_eq,
  less,
  less_eq,
  break_,
  continue_,
  index
};

struct Token {
  TokenType type;
  std::optional<std::string> value{};
  int line;
};

class Tokenizer {
public:
  Tokenizer(std::string str) : m_str(std::move(str)), m_index(0), m_line(1) {}

  std::vector<Token> tokenize();

private:
  const std::string m_str;
  size_t m_index;
  int m_line;
  char peek(int offset = 0) const;
  char consume();
};
