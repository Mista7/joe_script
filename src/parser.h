#pragma once
#include "lexer.h"
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class Node_Type {
  root,
  if_,
  elif_,
  else_,
  while_,
  for_,
  bin_expr,
  ret,
  print_,
  var,
  type,
  str_lit,
  char_lit,
  int_lit,
  bool_lit,
  float_lit,
};

class Node {
public:
  Node(Node_Type n) : m_type(n) {}
  virtual ~Node() = default;
  Node_Type m_type;

private:
};

// call Root_Node(Node_Type::root)
class Root_Node : public Node {
public:
  Root_Node() : Node(Node_Type::root) {}
  std::vector<std::unique_ptr<Node>> m_children;

private:
};

class If_Node : public Node {
public:
  If_Node(std::unique_ptr<Node> cond)
      : Node(Node_Type::if_), m_condition(std::move(cond)) {}
  std::unique_ptr<Node> m_condition;
  std::vector<std::unique_ptr<Node>> m_body;
  std::optional<std::unique_ptr<Node>> m_next;

private:
};

class Elif_Node : public Node {
public:
  Elif_Node(std::unique_ptr<Node> cond)
      : Node(Node_Type::elif_), m_condition(std::move(cond)) {}
  std::unique_ptr<Node> m_condition;
  std::vector<std::unique_ptr<Node>> m_body;
  std::optional<std::unique_ptr<Node>> m_next;

private:
};

class Else_Node : public Node {
public:
  Else_Node() : Node(Node_Type::else_) {}
  std::vector<std::unique_ptr<Node>> m_body;

private:
};

class Return_Node : public Node {
public:
  Return_Node(std::unique_ptr<Node> val)
      : Node(Node_Type::ret), m_value(std::move(val)) {}
  std::unique_ptr<Node> m_value;

private:
};

class While_Node : public Node {
public:
  While_Node(std::unique_ptr<Node> cond)
      : Node(Node_Type::while_), m_condition(std::move(cond)) {}

  std::unique_ptr<Node> m_condition;
  std::vector<std::unique_ptr<Node>> m_body;
  std::optional<std::unique_ptr<Else_Node>> m_else;

private:
};

class For_Node : public Node {
public:
  For_Node(std::unique_ptr<Node> init, std::unique_ptr<Node> cond,
           std::unique_ptr<Node> incr)
      : Node(Node_Type::for_), m_init(std::move(init)),
        m_condition(std::move(cond)), m_incr(std::move(incr)) {}

  std::unique_ptr<Node> m_init;
  std::unique_ptr<Node> m_condition;
  std::unique_ptr<Node> m_incr;
  std::vector<std::unique_ptr<Node>> m_body;
  std::optional<std::unique_ptr<Else_Node>> m_else;

private:
};

class Print_Node : public Node {
public:
  Print_Node(std::unique_ptr<Node> child)
      : Node(Node_Type::print_), m_child(std::move(child)) {}
  std::unique_ptr<Node> m_child;

private:
};

class Strlit_Node : public Node {
public:
  Strlit_Node(std::string val) : Node(Node_Type::str_lit), m_val(val) {}
  std::string m_val;

private:
};

class Intlit_Node : public Node {
public:
  Intlit_Node(int val) : Node(Node_Type::int_lit), m_val(val) {}
  int m_val;

private:
};

class Boollit_Node : public Node {
public:
  Boollit_Node(bool val) : Node(Node_Type::bool_lit), m_val(val) {}
  bool m_val;

private:
};

class Charlit_Node : public Node {
public:
  Charlit_Node(char val) : Node(Node_Type::char_lit), m_val(val) {}
  char m_val;

private:
};

class Floatlit_Node : public Node {
public:
  Floatlit_Node(float val) : Node(Node_Type::float_lit), m_val(val) {}
  float m_val;

private:
};
class Var_Node : public Node {
public:
  Var_Node(TokenType type, std::string name, std::unique_ptr<Node> val)
      : Node(Node_Type::var), m_var_type(type), m_name(name),
        m_val(std::move(val)) {}

  TokenType m_var_type;
  std::string m_name;
  std::unique_ptr<Node> m_val;

private:
};

class Expression_Node : public Node {
public:
  Expression_Node(std::unique_ptr<Node> left, TokenType op,
                  std::unique_ptr<Node> right)
      : Node(Node_Type::bin_expr), m_left(std::move(left)), m_op(op),
        m_right(std::move(right)) {}

  std::unique_ptr<Node> m_left;
  TokenType m_op;
  std::unique_ptr<Node> m_right;

private:
};

class Parser {
public:
  Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)), m_index(0) {}

  std::vector<std::unique_ptr<Root_Node>> parser();

private:
  int m_index;
  const std::vector<Token> m_tokens;
  std::optional<Token> peek(int offset = 0) const;
  Token consume();
};
