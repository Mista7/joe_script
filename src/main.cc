#include "lexer.h"
#include "parser.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

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
