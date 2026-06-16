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
  body,
  bin_expr,
  un_expr,
  ret,
  print_,
  var,
  var_ref,
  type,
  str_lit,
  char_lit,
  int_lit,
  bool_lit,
  float_lit,
};

class Node {
public:
  Node(Node_Type n) : m_type(n) {};
  virtual ~Node() = default;
  Node_Type m_type;
  // virtual Value *codegen() = 0;

private:
};

// call Root_Node(Node_Type::root)
class Root_Node : public Node {
public:
  Root_Node() : Node(Node_Type::root) {};
  std::vector<std::unique_ptr<Node>> m_children;
  // virtual Value *codegen() override;

private:
};

class If_Node : public Node {
public:
  If_Node(std::unique_ptr<Node> cond, std::unique_ptr<Node> body)
      : Node(Node_Type::if_), m_condition(std::move(cond)),
        m_body(std::move(body)) {};
  std::unique_ptr<Node> m_condition;
  std::unique_ptr<Node> m_body;
  std::optional<std::unique_ptr<Node>> m_next;

  // virtual Value *codegen() override;

private:
};

class Elif_Node : public Node {
public:
  Elif_Node(std::unique_ptr<Node> cond, std::unique_ptr<Node> body)
      : Node(Node_Type::elif_), m_condition(std::move(cond)),
        m_body(std::move(body)) {};
  std::unique_ptr<Node> m_condition;
  std::unique_ptr<Node> m_body;
  std::optional<std::unique_ptr<Node>> m_next;
  // virtual Value *codegen() override;

private:
};

class Else_Node : public Node {
public:
  Else_Node(std::unique_ptr<Node> body)
      : Node(Node_Type::else_), m_body(std::move(body)) {};
  std::unique_ptr<Node> m_body;
  // virtual Value *codegen() override;

private:
};

class Return_Node : public Node {
public:
  Return_Node(std::unique_ptr<Node> val)
      : Node(Node_Type::ret), m_value(std::move(val)) {};
  std::unique_ptr<Node> m_value;
  // virtual Value *codegen() override;

private:
};

class While_Node : public Node {
public:
  While_Node(std::unique_ptr<Node> cond, std::unique_ptr<Node> body)
      : Node(Node_Type::while_), m_condition(std::move(cond)),
        m_body(std::move(body)) {};

  std::unique_ptr<Node> m_condition;
  std::unique_ptr<Node> m_body;
  std::optional<std::unique_ptr<Else_Node>> m_else;
  // virtual Value *codegen() override;

private:
};

class For_Node : public Node {
public:
  For_Node(std::unique_ptr<Node> init, std::unique_ptr<Node> cond,
           std::unique_ptr<Node> incr, std::unique_ptr<Node> body)
      : Node(Node_Type::for_), m_init(std::move(init)),
        m_condition(std::move(cond)), m_incr(std::move(incr)),
        m_body(std::move(body)) {};

  std::unique_ptr<Node> m_init;
  std::unique_ptr<Node> m_condition;
  std::unique_ptr<Node> m_incr;
  std::unique_ptr<Node> m_body;
  std::optional<std::unique_ptr<Else_Node>> m_else;
  // virtual Value *codegen() override;

private:
};

class Body_Node : public Node {
public:
  Body_Node(std::vector<std::unique_ptr<Node>> items)
      : Node(Node_Type::body), m_items(std::move(items)) {};

  std::vector<std::unique_ptr<Node>> m_items;
  // virtual Value *codegen() override;

private:
};

class Print_Node : public Node {
public:
  Print_Node(std::unique_ptr<Node> child)
      : Node(Node_Type::print_), m_child(std::move(child)) {};
  std::unique_ptr<Node> m_child;
  // virtual Value *codegen() override;

private:
};

class Strlit_Node : public Node {
public:
  Strlit_Node(const std::string &val) : Node(Node_Type::str_lit), m_val(val) {};
  std::string m_val;
  // virtual Value *codegen() override;

private:
};

class Intlit_Node : public Node {
public:
  Intlit_Node(int val) : Node(Node_Type::int_lit), m_val(val) {};
  int m_val;
  // virtual Value *codegen() override;

private:
};

class Boollit_Node : public Node {
public:
  Boollit_Node(bool val) : Node(Node_Type::bool_lit), m_val(val) {};
  bool m_val;
  // virtual Value *codegen() override;

private:
};

class Charlit_Node : public Node {
public:
  Charlit_Node(char val) : Node(Node_Type::char_lit), m_val(val) {};
  char m_val;
  // virtual Value *codegen() override;

private:
};

class Floatlit_Node : public Node {
public:
  Floatlit_Node(float val) : Node(Node_Type::float_lit), m_val(val) {};
  float m_val;
  // virtual Value *codegen() override;

private:
};

class Var_Node : public Node {
public:
  Var_Node(const std::string &name, std::unique_ptr<Node> val, TokenType type,
           bool assigned = true)
      : Node(Node_Type::var), m_name(name), m_val(std::move(val)), m_type(type),
        m_assigned(assigned) {};

  std::string m_name;
  std::unique_ptr<Node> m_val;
  TokenType m_type;
  bool m_assigned;
  // virtual Value *codegen() override;

private:
};

class VarRef_Node : public Node {
public:
  VarRef_Node(const std::string &name)
      : Node(Node_Type::var_ref), m_name(name) {};
  std::string m_name;
  // virtual Value *codegen() override;

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
  // virtual Value *codegen() override;

private:
};

class Unary_Node : public Node {
public:
  Unary_Node(TokenType op, std::unique_ptr<Node> right)
      : Node(Node_Type::un_expr), m_op(op), m_right(std::move(right)) {}

  TokenType m_op;
  std::unique_ptr<Node> m_right;
  // virtual Value *codegen() override;

private:
};

class Parser {
public:
  Parser(std::vector<Token> tokens) : m_index(0), m_tokens(std::move(tokens)) {}

  std::unique_ptr<Root_Node> parser();

private:
  size_t m_index;
  const std::vector<Token> m_tokens;
  std::optional<Token> peek(int offset = 0) const;
  Token consume();
  std::unique_ptr<Return_Node> parse_ret();
  std::unique_ptr<Node> parse_expr(int min_bp = 0);
  std::unique_ptr<Node> parse_prefix(Token token);
  std::unique_ptr<Node> parse_infix(std::unique_ptr<Node> left, Token op);
  std::unique_ptr<Body_Node> parse_body();
  std::unique_ptr<Node> parse_if(Node_Type if_or_elif);
  std::unique_ptr<Node> parse_else();
  std::unique_ptr<Node> parse_for();
  std::unique_ptr<Node> parse_while();
  std::unique_ptr<Node> parse_print();
  std::unique_ptr<Node> parse_var(TokenType var_type);
};
