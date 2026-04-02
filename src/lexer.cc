#include "lexer.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

char Tokenizer::peek(int offset) const {
  if (m_str.size() <= m_index + offset) {
    return '\0';
  }
  char peek = m_str.at(m_index + offset);
  return peek;
}

char Tokenizer::consume() {
  char consumed = m_str.at(m_index);
  if (consumed == '\n') {
    m_line++;
  }
  m_index++;
  return consumed;
}

std::vector<Token> Tokenizer::tokenize() {
  std::vector<Token> tokens{};
  struct Token curr_token{};

  while (Tokenizer::peek() != '\0') {
    // If token starts or contains letters or nums
    if (std::isalpha(peek())) {
      curr_token.line = m_line;
      curr_token.value = std::string(1, peek());
      consume();

      while (std::isalnum(peek())) {
        (*curr_token.value) += std::string(1, peek());
        consume();
      }

      if (curr_token.value == "joeReturn") {
        curr_token.type = TokenType::return_;
      } else if (curr_token.value == "joePrint") {
        curr_token.type = TokenType::print;
      } else if (curr_token.value == "joeWhile") {
        curr_token.type = TokenType::while_;
      } else if (curr_token.value == "joeFor") {
        curr_token.type = TokenType::for_;
      } else if (curr_token.value == "joeElse") {
        curr_token.type = TokenType::else_;
      } else if (curr_token.value == "joeElif") {
        curr_token.type = TokenType::elif_;
      } else if (curr_token.value == "joeIf") {
        curr_token.type = TokenType::_if;
      } else if (curr_token.value == "joeBreak") {
        curr_token.type = TokenType::break_;
      } else if (curr_token.value == "joeContinue") {
        curr_token.type = TokenType::continue_;
      } else if (curr_token.value == "joeStr") {
        curr_token.type = TokenType::str_;
      } else if (curr_token.value == "joeInt") {
        curr_token.type = TokenType::int_;
      } else if (curr_token.value == "joeBool") {
        curr_token.type = TokenType::bool_;
      } else if (curr_token.value == "joeArr") {
        curr_token.type = TokenType::arr_;
      } else if (curr_token.value == "joeFloat") {
        curr_token.type = TokenType::float_;
      }

      else {
        curr_token.type = TokenType::var;
      }

      tokens.push_back(curr_token);
      curr_token = Token{};
    }

    // Ints
    else if (isdigit(peek())) {
      curr_token.value = std::string(1, peek());
      curr_token.line = m_line;
      consume();
      while (isdigit(peek())) {
        (*curr_token.value) += std::string(1, peek());
        consume();
      }
      curr_token.type = TokenType::int_lit;
      tokens.push_back(curr_token);
      curr_token = Token{};
    }

    else if (peek() == '(') {
      curr_token.line = m_line;
      curr_token.type = TokenType::round_open;
      curr_token.value = std::string(1, '(');
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();
    }

    else if (peek() == ')') {
      curr_token.line = m_line;
      curr_token.type = TokenType::round_close;
      curr_token.value = std::string(1, ')');
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();
    }

    else if (peek() == '{') {
      curr_token.line = m_line;
      curr_token.type = TokenType::curl_open;
      curr_token.value = std::string(1, '{');
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();
    }

    else if (peek() == '}') {
      curr_token.line = m_line;
      curr_token.type = TokenType::curl_close;
      curr_token.value = std::string(1, '}');
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();
    }

    else if (peek() == '"') {
      consume();
      curr_token.value = "";
      while (!(peek() == '"')) {
        if (peek() == '\0') {
          std::cerr << "Quote not closed, error on line " << m_line
                    << std::endl;
          exit(1);
        }
        (*curr_token.value) += std::string(1, consume());
      }

      curr_token.line = m_line;
      curr_token.type = TokenType::str_lit;
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();
    }

    // Characters
    else if (peek() == '\'') {
      consume();
      curr_token.value = "";
      if (peek(1) != '\'' || peek() == '\0' || peek() == '\n') {
        std::cerr << "Char quote not closed or too long, error on line "
                  << m_line << std::endl;
        exit(1);
      } else {
        (*curr_token.value) += std::string(1, consume());
      }
      curr_token.line = m_line;
      curr_token.type = TokenType::char_lit;
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();
    }

    // Punctuation or assignment
    else if (peek() == '!') {
      consume();
      if (peek() == '=') {
        curr_token.value = "!=";
        curr_token.type = TokenType::not_eq_;
        curr_token.line = m_line;
        consume();
      } else {
        curr_token.value = "!";
        curr_token.type = TokenType::not_;
        curr_token.line = m_line;
      }
      tokens.push_back(curr_token);
      curr_token = Token{};
    } else if (peek() == '*') {
      consume();
      if (peek() == '=') {
        curr_token.value = "*=";
        curr_token.type = TokenType::star_eq;
        curr_token.line = m_line;
        consume();
      } else {
        curr_token.value = "*";
        curr_token.type = TokenType::star;
        curr_token.line = m_line;
      }
      tokens.push_back(curr_token);
      curr_token = Token{};
    } else if (peek() == '/') {
      consume();
      if (peek() == '=') {
        curr_token.value = "/=";
        curr_token.type = TokenType::slash_eq;
        curr_token.line = m_line;
        consume();
      } else if (peek() == '/') {
        consume();
        while (!(peek() == '\0')) {
          (*curr_token.value) += std::string(1, consume());
        }
        curr_token.type = TokenType::comment;
        curr_token.line = m_line;
        consume();
        tokens.push_back(curr_token);
        curr_token = Token{};

      } else {
        curr_token.value = "/";
        curr_token.type = TokenType::slash;
        curr_token.line = m_line;
      }
      tokens.push_back(curr_token);
      curr_token = Token{};
    } else if (peek() == '-') {
      consume();
      if (peek() == '=') {
        curr_token.value = "-=";
        curr_token.type = TokenType::minus_eq;
        curr_token.line = m_line;
        consume();
      } else {
        curr_token.value = "-";
        curr_token.type = TokenType::minus;
        curr_token.line = m_line;
      }
      tokens.push_back(curr_token);
      curr_token = Token{};
    } else if (peek() == '+') {
      consume();
      if (peek() == '=') {
        curr_token.value = "+=";
        curr_token.type = TokenType::plus_eq;
        curr_token.line = m_line;
        consume();
      } else {
        curr_token.value = "+";
        curr_token.type = TokenType::plus;
        curr_token.line = m_line;
      }
      tokens.push_back(curr_token);
      curr_token = Token{};
    } else if (peek() == '=') {
      consume();
      if (peek() == '=') {
        curr_token.value = "==";
        curr_token.type = TokenType::equal;
        curr_token.line = m_line;
        consume();
      } else {
        curr_token.value = "=";
        curr_token.type = TokenType::assign;
        curr_token.line = m_line;
        consume();
      }
      tokens.push_back(curr_token);
      curr_token = Token{};
    } else if (peek() == '>') {
      consume();
      if (peek() == '=') {
        curr_token.value = ">=";
        curr_token.type = TokenType::great_eq;
        curr_token.line = m_line;
        consume();
      } else {
        curr_token.value = ">";
        curr_token.type = TokenType::great;
        curr_token.line = m_line;
        consume();
      }
      tokens.push_back(curr_token);
      curr_token = Token{};
    } else if (peek() == '<') {
      consume();
      if (peek() == '=') {
        curr_token.value = "<=";
        curr_token.type = TokenType::less_eq;
        curr_token.line = m_line;
        consume();
      } else {
        curr_token.value = "<";
        curr_token.type = TokenType::less;
        curr_token.line = m_line;
        consume();
      }
      tokens.push_back(curr_token);
      curr_token = Token{};
    }

    // Literals
    else if (peek() == ';') {
      curr_token.type = TokenType::semi;
      curr_token.value = ";";
      curr_token.line = m_line;
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();

    } else if (peek() == ',') {
      curr_token.type = TokenType::comma;
      curr_token.value = ",";
      curr_token.line = m_line;
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();

    } else if (peek() == '.') {
      curr_token.type = TokenType::dot;
      curr_token.value = ".";
      curr_token.line = m_line;
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();

    } else if (peek() == '[') {
      curr_token.type = TokenType::square_open;
      curr_token.value = "[";
      curr_token.line = m_line;
      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();

      curr_token.value = "";
      curr_token.line = m_line;

      while (peek() != ']') {
        if (peek() == '\0') {
          std::cerr << "No closing square bracket smh, error on line " << m_line
                    << std::endl;
          exit(1);
        } else if (!isdigit(peek())) {
          std::cerr << "Not a valid index, error on line " << m_line
                    << std::endl;
          exit(1);
        }
        (*curr_token.value) += std::string(1, consume());
      }

      curr_token.type = TokenType::index;
      tokens.push_back(curr_token);
      curr_token = Token{};

      curr_token.type = TokenType::square_close;
      curr_token.value = "]";
      curr_token.line = m_line;

      tokens.push_back(curr_token);
      curr_token = Token{};
      consume();
    }

    else if (isspace(peek())) {
      consume();
    }

    else {
      std::cerr
          << "What sped characters are u using bruv, what da helly is this: "
          << peek() << "this on line " << m_line << std::endl;
      exit(1);
    }
  }

  return tokens;
}

int main() {
  std::ifstream inputfile("test.txt");

  if (!inputfile.is_open()) {
    std::cout << "ERROR";
    return 1;
  }

  std::string line;
  std::string file;

  while (std::getline(inputfile, line)) {
    file += line;
    file += '\n';
  }

  std::cout << file << std::endl;

  Tokenizer tokenize_file(file);
  std::vector<Token> tokens = tokenize_file.tokenize();

  for (auto &token : tokens) {
    std::cout << "Value: " << token.value.value_or("no value")
              << " Line: " << token.line << std::endl;
  }
  return 0;
}
