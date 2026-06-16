#include "parser.h"
#include "lexer.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

std::optional<Token> Parser::peek(int offset) const {
  if (m_tokens.size() <= (offset + m_index)) {
    return std::nullopt;
  }
  return m_tokens.at(m_index + offset);
}

Token Parser::consume() {
  m_index++;
  return Parser::m_tokens.at(m_index - 1);
}

/////////////////////////////////////////////////////////////////////////
// Outputs the AST via the Root Node
std::unique_ptr<Root_Node> Parser::parser() {
  auto root = std::make_unique<Root_Node>();
  while (m_tokens.size() != m_index) {

    std::cout << "m_tokens size: " << m_tokens.size() << " m_index: " << m_index
              << std::endl;

    if (peek().has_value()) {
      TokenType type = peek()->type;
      if (type == TokenType::return_) {
        root->m_children.push_back(parse_ret());
      }

      else if (type == TokenType::float_ || type == TokenType::int_ ||
               type == TokenType::bool_ || type == TokenType::arr_ ||
               type == TokenType::str_) {
        root->m_children.push_back(parse_var(consume().type));
      }

      else if (type == TokenType::if_) {
        root->m_children.push_back(parse_if(Node_Type::if_));
      }

      else if (type == TokenType::elif_) {
        root->m_children.push_back(parse_if(Node_Type::elif_));
      }

      else if (type == TokenType::else_) {
        root->m_children.push_back(parse_else());
      }

      else if (type == TokenType::while_) {
        root->m_children.push_back(parse_while());
      }

      else if (type == TokenType::for_) {
        root->m_children.push_back(parse_for());
      }

      else if (type == TokenType::print) {
        root->m_children.push_back(parse_print());
      } else {
        std::cerr << "Unknown token type: " << static_cast<int>(type)
                  << std::endl;
        return nullptr;
      }
    }
  }

  return root;
}

/////////////////////////////////////////////////////////////////////////
// Parses for Return Node
std::unique_ptr<Return_Node> Parser::parse_ret() {
  consume();
  auto val = parse_expr();
  if (peek().has_value() && peek()->type == TokenType::semi) {
    consume();
  } else {
    std::cerr << "Invalid return statement" << std::endl;
  }

  return std::make_unique<Return_Node>(std::move(val));
}

/////////////////////////////////////////////////////////////////////////
// Outputs the binding power for infix tokens
int infix_bp(Token token) {
  switch (token.type) {
  case TokenType::plus:
    return 10;
  case TokenType::minus:
    return 10;
  case TokenType::star:
    return 13;
  case TokenType::slash:
    return 13;
  case TokenType::exp:
    return 16;
  case TokenType::equal:
    return 7;
  case TokenType::not_eq_:
    return 7;
  case TokenType::great:
    return 8;
  case TokenType::great_eq:
    return 8;
  case TokenType::less:
    return 8;
  case TokenType::less_eq:
    return 8;
  case TokenType::logical_and:
    return 4;
  case TokenType::logical_or:
    return 3;
  case TokenType::assign:
    return 1;
  case TokenType::plus_eq:
    return 1;
  case TokenType::minus_eq:
    return 1;
  case TokenType::slash_eq:
    return 1;
  case TokenType::star_eq:
    return 1;
  default:
    return 0;
  }
}

/////////////////////////////////////////////////////////////////////////
// Outputs the binding power for infix tokens
int prefix_bp(Token token) {
  if (token.type == TokenType::minus || token.type == TokenType::not_ ||
      token.type == TokenType::minus_minus ||
      token.type == TokenType::plus_plus) {
    return 15;
  }
  return -1;
}

/////////////////////////////////////////////////////////////////////////
// Parse the prefix of an expression
std::unique_ptr<Node> Parser::parse_prefix(Token token) {
  if (token.type == TokenType::int_lit) {
    return std::make_unique<Intlit_Node>(std::stoi(token.value.value()));
  }

  else if (token.type == TokenType::float_lit) {
    return std::make_unique<Floatlit_Node>(std::stof(token.value.value()));
  }

  else if (token.type == TokenType::true_) {
    return std::make_unique<Boollit_Node>(true);
  }

  else if (token.type == TokenType::false_) {
    return std::make_unique<Boollit_Node>(false);
  }

  else if (token.type == TokenType::str_lit) {
    return std::make_unique<Strlit_Node>(token.value.value());
  }

  else if (token.type == TokenType::char_lit) {
    return std::make_unique<Charlit_Node>(token.value.value()[0]);
  }

  else if (token.type == TokenType::minus || token.type == TokenType::not_ ||
           token.type == TokenType::plus_plus ||
           token.type == TokenType::minus_minus) {
    std::unique_ptr<Node> right = parse_expr(prefix_bp(token));
    return std::make_unique<Unary_Node>(token.type, std::move(right));
  }

  else if (token.type == TokenType::round_open) {
    std::unique_ptr<Node> right = parse_expr(0);
    if (peek().has_value() && peek().value().type == TokenType::round_close) {
      consume();
    } else {
      std::cerr << "Missing closing bracket monkey" << std::endl;
      return nullptr;
    }
    return right;
  }

  else if (token.type == TokenType::var) {
    return std::make_unique<VarRef_Node>(token.value.value());
  }

  else {
    std::cerr << "Expected Expression but got monkeyness" << std::endl;
    return nullptr;
  }
}

/////////////////////////////////////////////////////////////////////////
// Parses infix of an expression
std::unique_ptr<Node> Parser::parse_infix(std::unique_ptr<Node> left,
                                          Token op) {
  int op_bp = infix_bp(op);
  std::unique_ptr<Node> right;

  if (op_bp == 1 || op.type == TokenType::exp) {
    right = parse_expr(op_bp - 1);
  } else {
    right = parse_expr(op_bp);
  }
  return std::make_unique<Expression_Node>(std::move(left), op.type,
                                           std::move(right));
}

/////////////////////////////////////////////////////////////////////////
// Parses an expression and returns the node
std::unique_ptr<Node> Parser::parse_expr(int min_bp) {
  std::unique_ptr<Node> left = parse_prefix(consume());
  while (peek().has_value() && infix_bp(peek().value()) > min_bp) {
    Token op = consume();
    std::unique_ptr<Node> node = parse_infix(std::move(left), op);
    left = std::move(node);
  }
  return left;
}

/////////////////////////////////////////////////////////////////////////
// Parses if or elif statements
std::unique_ptr<Node> Parser::parse_if(Node_Type if_or_elif) {
  consume();
  std::unique_ptr<Node> cond = parse_expr();
  std::unique_ptr<Node> body = parse_body();

  if (if_or_elif == Node_Type::if_) {
    return std::make_unique<If_Node>(std::move(cond), std::move(body));
  } else {
    return std::make_unique<Elif_Node>(std::move(cond), std::move(body));
  }
}

/////////////////////////////////////////////////////////////////////////
// Parses else
std::unique_ptr<Node> Parser::parse_else() {
  consume();
  std::unique_ptr<Node> body = parse_body();
  return std::make_unique<Else_Node>(std::move(body));
}

/////////////////////////////////////////////////////////////////////////
// Parses the body of statements/loops
std::unique_ptr<Body_Node> Parser::parse_body() {
  std::vector<std::unique_ptr<Node>> body;
  if (peek().has_value() && peek()->type == TokenType::curl_open) {
    consume();
  }
  while (peek().has_value() && peek()->type != TokenType::curl_close) {
    TokenType type = peek()->type;
    if (type == TokenType::if_) {
      body.push_back(parse_if(Node_Type::if_));
    } else if (type == TokenType::elif_) {
      body.push_back(parse_if(Node_Type::elif_));
    } else if (type == TokenType::else_) {
      body.push_back(parse_else());
    } else if (type == TokenType::for_) {
      body.push_back(parse_for());
    } else if (type == TokenType::while_) {
      body.push_back(parse_while());
    } else if (type == TokenType::return_) {
      body.push_back(parse_ret());
    } else if (type == TokenType::print) {
      body.push_back(parse_print());
    } else if (type == TokenType::float_ || type == TokenType::int_ ||
               type == TokenType::bool_ || type == TokenType::arr_ ||
               type == TokenType::str_) {
      body.push_back(parse_var(consume().type));
    }

    else {
      body.push_back(parse_expr());
      if (peek()->type == TokenType::semi) {
        consume();
      } else {
        std::cerr << "Invalid statement" << std::endl;
      }
    }
  }

  if (peek().has_value() && peek()->type == TokenType::curl_close) {

    consume();
  } else {
    std::cerr << "Missing close bracket" << std::endl;
  }

  return std::make_unique<Body_Node>(std::move(body));
}

std::unique_ptr<Node> Parser::parse_while() {
  consume();
  std::unique_ptr<Node> cond = parse_expr();
  std::unique_ptr<Node> body = parse_body();
  return std::make_unique<While_Node>(std::move(cond), std::move(body));
}

std::unique_ptr<Node> Parser::parse_for() {
  consume();
  if (peek().has_value() && peek()->type == TokenType::round_open) {
    consume();
  } else {
    std::cerr << "Missing Bracket in for" << std::endl;
    return nullptr;
  }

  TokenType type = peek()->type;
  std::unique_ptr<Node> init;
  if (type == TokenType::float_ || type == TokenType::int_ ||
      type == TokenType::bool_ || type == TokenType::arr_ ||
      type == TokenType::str_) {

    init = parse_var(consume().type);
  } else {
    std::cerr << "Invalid variable declaration in for loop" << std::endl;
    return nullptr;
  }

  // if (peek().has_value() && peek()->type == TokenType::semi) {
  //   consume();
  // } else {
  //   std::cerr << "Missing semicolon in for loop" << std::endl;
  //   return nullptr;
  // }

  std::unique_ptr<Node> cond = parse_expr();
  if (peek().has_value() && peek()->type == TokenType::semi) {
    consume();
  } else {
    std::cerr << "Missing semicolon in for loop" << std::endl;
    return nullptr;
  }
  std::unique_ptr<Node> incr = parse_expr();

  if (peek().has_value() && peek()->type == TokenType::round_close) {
    consume();
  } else {
    std::cerr << "Closing bracket missing" << std::endl;
    return nullptr;
  }

  std::unique_ptr<Node> body = parse_body();
  return std::make_unique<For_Node>(std::move(init), std::move(cond),
                                    std::move(incr), std::move(body));
}

std::unique_ptr<Node> Parser::parse_print() {
  consume();
  if (peek().has_value() && peek()->type == TokenType::round_open) {
    consume();
  } else {
    std::cerr << "Missing open bracket for print" << std::endl;
    return nullptr;
  }

  std::unique_ptr<Node> child = parse_expr();

  if (peek().has_value() && peek()->type == TokenType::round_close) {
    consume();
  } else {
    std::cerr << "Missing closing bracket for print" << std::endl;
    return nullptr;
  }

  if (peek().has_value() && peek()->type == TokenType::semi) {
    consume();
  } else {
    std::cerr << "missing semicolon after print" << std::endl;
    return nullptr;
  }

  return std::make_unique<Print_Node>(std::move(child));
}

std::unique_ptr<Node> Parser::parse_var(TokenType var_type) {
  if (peek().has_value() && peek()->type != TokenType::var) {
    std::cerr << "Invalid variable name gang" << std::endl;
    return nullptr;

  } else {
    Token var = consume();
    std::string var_name = var.value.value();
    if (peek().has_value() && peek()->type == TokenType::semi) {
      consume();
      return std::make_unique<Var_Node>(var_name, nullptr, var_type, false);
    }

    else if (peek().has_value() && peek()->type == TokenType::assign) {
      consume();
      std::unique_ptr<Node> val = parse_expr();

      if (peek()->type == TokenType::semi) {
        consume();
      } else {
        std::cerr << "Missing semicolon after variable declaration"
                  << std::endl;
      }
      return std::make_unique<Var_Node>(var_name, std::move(val), var_type);

    } else {
      std::cerr << "Improper assignment of variable" << std::endl;
      return nullptr;
    }
  }
}
