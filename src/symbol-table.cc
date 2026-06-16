#include "symbol-table.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <iostream>
#include <ostream>

void var_table::push_scope() {
  std::map<std::string, Var_Info> empty_map;
  m_table.push_back(empty_map);
}

void var_table::pop_scope() { m_table.pop_back(); }

bool var_table::is_redeclared(std::string name) {
  if (in_local(name) || in_global(name)) {
    return true;
  }
  return false;
}

void var_table::add_local(Var_Info var) {
  if (is_redeclared(var.m_name)) {
    std::cerr << "Variable has already been declared" << std::endl;
    return;
  }
  m_table.back()[var.m_name] = var;
  return;
}

void var_table::add_global(Var_Info var) {
  if (is_redeclared(var.m_name)) {
    std::cerr << "Variable has already been declared in scope" << std::endl;
    return;
  }
  m_table.at(0)[var.m_name] = var;
  return;
}

std::optional<Var_Info> var_table::search_var(std::string name) {

  for (auto it = m_table.rbegin(); it != m_table.rend(); it++) {
    auto var = it->find(name);
    if (var != it->end()) {
      return var->second;
    }
  }
  return std::nullopt;
}

bool var_table::in_local(std::string name) {
  if (m_table.back().find(name) != m_table.back().end()) {
    return true;
  }
  return false;
}

bool var_table::in_global(std::string name) {
  if (m_table.at(0).find(name) != m_table.at(0).end()) {
    return true;
  }
  return false;
}
