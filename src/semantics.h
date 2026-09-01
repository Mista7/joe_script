#pragma once
#include "lexer.h"
#include "parser.h"
#include "symbol-table.h"
#include <memory>

class Semantics {
public:
  Semantics(std::unique_ptr<Root_Node> root) : m_root(std::move(root)) {}

  void scan();
  void scan_statement(Node *child);
  void scan_var_decl(Node *node);
  Var_Info scan_var_ref(Node *node);
  void scan_if(Node *node);
  void scan_else(Node *node);
  void scan_while(Node *node);
  void scan_for(Node *node);
  TokenType scan_return(Node *node);
  void scan_body(Node *node);
  TokenType scan_expr(Node *node);
  TokenType scan_unary_expr(Node *node);
  void scan_print(Node *node);
  void scan_function_decl(Node *node);
  TokenType scan_function_call(Node *node);

  var_table vars;

private:
  std::unique_ptr<Root_Node> m_root;
};
