#include "parser.h"
#include "lexer.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <setjmp.h>
#include <stdexcept>
#include <string>
#include <vector>

std::optional<Token> Parser::peek(int offset) const {
  if (m_tokens.size() < (offset + m_index)) {
    return std::nullopt;
  }
  return m_tokens.at(m_index + offset);
}

Token Parser::consume() {
  m_index++;
  return Parser::m_tokens.at(m_index - 1);
}

std::vector<std::unique_ptr<Root_Node>> Parser::parser() {
  Root_Node root;
  while (m_tokens.size() != m_index) {
    if (peek().has_value()) {
      if (peek()->type == TokenType::return_) {
        TokenType type = peek()->type;
        root.m_children.push_back(std::move(parse_ret(type)));
      } else if (peek()->type == TokenType::if_) {
        consume();

        if (peek()->type == TokenType::round_open) {
          while (peek()->type != TokenType::round_close) {
            if (peek()->type == TokenType::round_open) {
              while (peek()->type != TokenType::round_close)
            }
          }
        }
        parse_if();
      }
    }
  }
}

std::unique_ptr<Return_Node> Parser::parse_ret(TokenType type) {
  std::unique_ptr<Node> val;
  switch (type) {
  case TokenType::int_lit:
    val = std::make_unique<Intlit_Node>(std::stoi(peek()->value.value()));
    break;
  case TokenType::char_lit:
    val = std::make_unique<Charlit_Node>(peek()->value.value()[0]);
    break;
  case TokenType::bool_:
    val = std::make_unique<Boollit_Node>(
        (peek()->value.value() == "true") ? true : false);
    break;
  case TokenType::str_lit:
    val = std::make_unique<Strlit_Node>(peek()->value.value());
    break;
  case TokenType::var:
    // VAR PARSING NOT MADE YET
    std::cout << "Need to implement variable parsing" << std::endl;
    break;
    // val = std::make_unique<Var_Node>()
  default:
    throw std::runtime_error("Not valid for a return expression");
  }
  consume();
  return std::make_unique<Return_Node>(std::move(val));
}

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

int prefix_bp(Token token) {
  if (token.type == TokenType::minus || token.type == TokenType::not_ ||
      token.type == TokenType::minus_minus ||
      token.type == TokenType::plus_plus) {
    return 15;
  }
  return 0;
}

std::unique_ptr<Node> Parser::parse_prefix(Token token) {
  if (token.type == TokenType::int_lit) {
    return std::make_unique<Intlit_Node>(std::stoi(token.value.value()));
  }

  else if (token.type == TokenType::float_lit) {
    return std::make_unique<Floatlit_Node>(std::stof(token.value.value()));
  }

  else if (token.type == TokenType::bool_lit) {
    return std::make_unique<Boollit_Node>(
        token.value.value() == "true" ? true : false);
  }

  else if (token.type == TokenType::str_lit) {
    return std::make_unique<Strlit_Node>(token.value.value());
  }

  else if (token.type == TokenType::char_) {
    return std::make_unique<Charlit_Node>(token.value.value()[0]);
  }

  else if (token.type == TokenType::minus || token.type == TokenType::not_ ||
           token.type == TokenType::plus_plus ||
           token.type == TokenType::minus_minus) {
    std::unique_ptr<Node> right = parse_expr(prefix_bp(token));
    return std::make_unique<Unary_Node>(token.type, std::move(right));
  }

  else if (token.type == TokenType::round_open) {
    consume();
    std::unique_ptr<Node> right = parse_expr(0);
    consume();
    return right;
  }

  else if (token.type == TokenType::var) {
    return std::make_unique<VarRef_Node>(token.value.value());
  }

  else {
    std::cerr << "Expected Expression but got monkeyness" << std::endl;
    return NULL;
  }
}

std::unique_ptr<Node> Parser::parse_infix(std::unique_ptr<Node> left,
                                          Token op) {
  int op_bp = infix_bp(op);
  std::unique_ptr<Node> right;

  if (op.type == TokenType::equal) {
    // right = parse_expr(op_bp - 1);
    //
    // if (left->m_type != Node_Type::var) {
    //   std::cerr << "Cannot assign to this type, err on line " << op.line
    //             << std::endl;
    // }
    //
    // return std::make_unique<Var_Node>()
  }
  right = parse_expr(op_bp);
  return std::make_unique<Expression_Node>(std::move(left), op.type,
                                           std::move(right));
}

std::unique_ptr<Node> Parser::parse_expr(int min_bp) {
  std::unique_ptr<Node> left = std::move(parse_prefix(consume()));
  while (peek().has_value() && infix_bp(peek().value()) > min_bp) {
    Token op = consume();
    std::unique_ptr<Node> node = parse_infix(std::move(left), op);
    left = std::move(node);
  }
  return left;
}
