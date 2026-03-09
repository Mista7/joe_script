#pragma once
#include <iostream>
#include <string.h>

enum class TokenTypes {
  joeReturn,
  joePrint,
  joeWhile,
  joeFor,
  joeElse,
  joeInt,
  joeStr,
  joeBool,
  joeFloat,
  joeIf,
  joeElif,
  joeClass,
  closeBrack,
  openBrack,
  curlOpen,
  curlClose,
  colon,
  semi,
  comma,
  asterisk,
  equal,
  dot,
  openQuote,
  closeQuote,
  plus,
  minus,
  slash,
  char
};

struct Token {
  TokenTypes type;
  std::optional<std::string> value{};
  int line;
};

class Tokenizer {
public:
  TokenTypes token;

private:
}
