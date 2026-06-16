#pragma once
#include "lexer.h"
#include "parser.h"
// #include "llvm/IR/IRBuilder.h"
// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

class Var_Info {
public:
  Var_Info(TokenType type, bool has_value, std::string name)
      : m_type(type), m_has_value(has_value), m_name(name) {};

  Var_Info() = default;

  TokenType m_type;
  bool m_has_value = false;
  std::string m_name;

private:
};

// enum class Scope {
//   global,
//   local
// };

class var_table {
public:
  var_table() { push_scope(); }
  void push_scope();
  void pop_scope();
  void add_global(Var_Info var); // Adds to global scope
  void add_local(Var_Info var);  // Adds to current scope
  std::optional<Var_Info> search_var(std::string name);
  bool in_local(std::string name);
  bool in_global(std::string name);
  bool is_redeclared(std::string name);

private:
  std::vector<std::map<std::string, Var_Info>> m_table;
  // void push_scope();
  // void pop_scope();
  // void add_global(std::string name, Var_Info var);
  // std::optional<Var_Info> search_var(std::string name);
  // bool in_local(std::string name);
  // bool in_global(std::string name);
  // void add_local(std::string name, Var_Info var); // Adds to current scope
  // bool is_redeclared(std::string name);
};
