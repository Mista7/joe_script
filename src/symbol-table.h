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

struct Func_Info {
  std::vector<TokenType> param_types;
  TokenType
      return_type; // Defaulting to int_ since your syntax omits return types
};

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
  void add_func(const std::string &name, Func_Info info) {
    m_funcs[name] = info;
  }
  std::optional<Func_Info> search_func(const std::string &name) {
    if (m_funcs.find(name) != m_funcs.end())
      return m_funcs[name];
    return std::nullopt;
  }
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
  std::map<std::string, Func_Info> m_funcs;
  // void push_scope();
  // void pop_scope();
  // void add_global(std::string name, Var_Info var);
  // std::optional<Var_Info> search_var(std::string name);
  // bool in_local(std::string name);
  // bool in_global(std::string name);
  // void add_local(std::string name, Var_Info var); // Adds to current scope
  // bool is_redeclared(std::string name);
};
