#include "semantics.h"
#include "lexer.h"
#include "parser.h"
#include "symbol-table.h"
#include <cstddef>
#include <iostream>

TokenType to_base_type(TokenType type);

// Name of a type the way the user writes it, for error messages
std::string type_name(TokenType type) {
  switch (to_base_type(type)) {
  case TokenType::int_:
    return "joeInt";
  case TokenType::float_:
    return "joeFloat";
  case TokenType::bool_:
    return "joeBool";
  case TokenType::str_:
    return "joeStr";
  case TokenType::char_:
    return "char";
  case TokenType::arr_:
    return "joeArr";
  default:
    return "unknown type";
  }
}

// Symbol of an operator, for error messages
std::string op_name(TokenType op) {
  switch (op) {
  case TokenType::plus:
    return "+";
  case TokenType::minus:
    return "-";
  case TokenType::star:
    return "*";
  case TokenType::slash:
    return "/";
  case TokenType::exp:
    return "**";
  case TokenType::equal:
    return "==";
  case TokenType::not_eq_:
    return "!=";
  case TokenType::great:
    return ">";
  case TokenType::great_eq:
    return ">=";
  case TokenType::less:
    return "<";
  case TokenType::less_eq:
    return "<=";
  case TokenType::logical_and:
    return "&&";
  case TokenType::logical_or:
    return "||";
  case TokenType::assign:
    return "=";
  case TokenType::plus_eq:
    return "+=";
  case TokenType::minus_eq:
    return "-=";
  case TokenType::star_eq:
    return "*=";
  case TokenType::slash_eq:
    return "/=";
  case TokenType::plus_plus:
    return "++";
  case TokenType::minus_minus:
    return "--";
  case TokenType::not_:
    return "!";
  default:
    return "?";
  }
}

std::ostream &Semantics::error(int line) {
  ++m_error_count;
  return std::cerr << "Error on line " << line << ": ";
}

void Semantics::scan() {
  std::size_t i = 0;
  while (i < m_root->m_children.size()) {
    auto child = m_root->m_children[i].get();
    scan_statement(child);
    i++;
  }
}

void Semantics::scan_function_decl(Node *node) {
  auto func = static_cast<FunctionDecl_Node *>(node);

  // 1. Build the signature and register it globally
  Func_Info info;
  info.return_type = func->m_return_type;
  for (const auto &param : func->m_params) {
    info.param_types.push_back(param.first);
  }

  if (vars.search_func(func->m_name).has_value()) {
    error(func->m_line) << "Function '" << func->m_name
                        << "' is already declared. Pick a different name, we "
                           "don't do duplicates here gang."
                        << std::endl;
    return;
  }
  vars.add_func(func->m_name, info);

  // 2. Push a new scope for the function body
  vars.push_scope();

  // 3. Add parameters to the local scope as assigned variables
  for (const auto &param : func->m_params) {
    if (vars.is_redeclared(param.second)) {
      error(func->m_line) << "Parameter '" << param.second << "' of '"
                          << func->m_name
                          << "' is already used as a variable or parameter "
                             "name. Rename it, the compiler can't read your "
                             "mind."
                          << std::endl;
      continue;
    }
    Var_Info param_var(param.first, true, param.second);
    vars.add_local(param_var);
  }

  // 4. Scan the body
  scan_body(func->m_body.get());

  // 5. Pop the scope
  vars.pop_scope();
}

TokenType Semantics::scan_function_call(Node *node) {
  auto call = static_cast<FunctionCall_Node *>(node);

  auto opt_func = vars.search_func(call->m_name);
  if (!opt_func.has_value()) {
    error(call->m_line) << "Function '" << call->m_name
                        << "' doesn't exist. Either you spelled it wrong or "
                           "you called it before declaring it. Functions have "
                           "to be declared ABOVE where you call them son."
                        << std::endl;
    return TokenType::err;
  }

  Func_Info info = opt_func.value();

  if (info.param_types.size() != call->m_args.size()) {
    error(call->m_line) << "'" << call->m_name << "' takes "
                        << info.param_types.size() << " argument(s) but you "
                        << "gave it " << call->m_args.size()
                        << ". Count again bro." << std::endl;
    return TokenType::err;
  }

  for (size_t i = 0; i < call->m_args.size(); i++) {
    TokenType arg_type = scan_expr(call->m_args[i].get());
    if (arg_type == TokenType::err) {
      continue; // Already complained about this argument
    }
    if (to_base_type(arg_type) != to_base_type(info.param_types[i])) {
      error(call->m_line) << "Argument " << i + 1 << " of '" << call->m_name
                          << "' should be a " << type_name(info.param_types[i])
                          << " but you passed a " << type_name(arg_type)
                          << ". Read the function signature gang." << std::endl;
    }
  }

  return info.return_type;
}

void Semantics::scan_statement(Node *child) {
  auto type = child->m_type;
  if (type == Node_Type::if_ || type == Node_Type::elif_) {
    scan_if(child);
  }

  else if (type == Node_Type::func_decl) {
    scan_function_decl(child);
  }

  else if (type == Node_Type::while_) {
    scan_while(child);
  }

  else if (type == Node_Type::for_) {
    scan_for(child);
  }

  else if (type == Node_Type::bin_expr) {
    scan_expr(child);
  }

  else if (type == Node_Type::un_expr) {
    scan_unary_expr(child);
  }

  else if (type == Node_Type::var) {
    scan_var_decl(child);
  }

  else if (type == Node_Type::var_ref) {
    scan_var_ref(child);
  }

  else if (type == Node_Type::body) {
    scan_body(child);
  }

  else if (type == Node_Type::ret) {
    scan_return(child);
  }

  else if (type == Node_Type::print_) {
    scan_print(child);
  }

  else if (type == Node_Type::func_call) {
    scan_function_call(child);
  }

  else if (type == Node_Type::else_) {
    scan_else(child);
  }

  else {
    error(child->m_line)
        << "This line is just a value sitting there doing nothing son. Did "
           "you mean to joePrint it or store it in a variable?"
        << std::endl;
  }
};

TokenType to_base_type(TokenType type) {
  if (type == TokenType::int_lit)
    return TokenType::int_;
  if (type == TokenType::float_lit)
    return TokenType::float_;
  if (type == TokenType::bool_lit)
    return TokenType::bool_;
  if (type == TokenType::str_lit)
    return TokenType::str_;
  if (type == TokenType::char_lit)
    return TokenType::char_;
  return type;
}

void Semantics::scan_var_decl(Node *node) {
  auto var = static_cast<Var_Node *>(node);

  std::string name = var->m_name;
  bool assigned = var->m_assigned;
  TokenType type = var->m_type;

  if (vars.is_redeclared(name)) {
    error(var->m_line) << "Variable '" << name
                       << "' has already been declared. If you want to change "
                          "it, drop the type and just do "
                       << name << " = ...; ez." << std::endl;
  } else {
    if (assigned) {
      TokenType val_type = scan_expr(var->m_val.get());
      // err means the value already got roasted, don't pile on
      if (val_type != TokenType::err && to_base_type(val_type) != type) {
        error(var->m_line) << "'" << name << "' is a " << type_name(type)
                           << " but you're trying to put a "
                           << type_name(val_type)
                           << " in it. That's not how types work chief."
                           << std::endl;
      }
    }
    // Still add it so every later use doesn't also say it's undeclared
    auto info = Var_Info(type, assigned, name);
    vars.add_local(info);
  }
};

Var_Info Semantics::scan_var_ref(Node *node) {
  auto ref = static_cast<VarRef_Node *>(node);
  auto info = vars.search_var(ref->m_name);

  if (!info.has_value()) {
    error(ref->m_line) << "'" << ref->m_name
                       << "' hasn't been declared. You can't use a variable "
                          "that doesn't exist bro. Declare it first (e.g. "
                          "joeInt "
                       << ref->m_name
                       << " = 0;) or check your spelling, it happens to the "
                          "worst of us."
                       << std::endl;
    return Var_Info(TokenType::err, false, ref->m_name);
  }

  return info.value();
};

void Semantics::scan_if(Node *node) {
  auto if_node = static_cast<If_Node *>(node);
  auto cond = if_node->m_condition.get();
  auto body = if_node->m_body.get();

  TokenType cond_type = scan_expr(cond);
  if (cond_type != TokenType::bool_ && cond_type != TokenType::bool_lit &&
      cond_type != TokenType::err) {
    error(node->m_line) << "TS not a proper condition son. "
                        << (node->m_type == Node_Type::elif_ ? "joeElif"
                                                             : "joeIf")
                        << " needs a joeBool (like x > 3 or x == y) but this "
                           "is a "
                        << type_name(cond_type) << "." << std::endl;
  }
  vars.push_scope();
  scan_body(body);
  vars.pop_scope();
};

void Semantics::scan_else(Node *node) {
  auto else_node = static_cast<Else_Node *>(node);
  auto body = else_node->m_body.get();

  vars.push_scope();
  scan_body(body);
  vars.pop_scope();
}

void Semantics::scan_while(Node *node) {
  auto while_node = static_cast<While_Node *>(node);
  auto cond = while_node->m_condition.get();
  auto body = while_node->m_body.get();

  TokenType cond_type = scan_expr(cond);
  if (cond_type != TokenType::bool_ && cond_type != TokenType::bool_lit &&
      cond_type != TokenType::err) {
    error(node->m_line) << "TS not a proper condition son. joeWhile needs a "
                           "joeBool (like count <= 3) but this is a "
                        << type_name(cond_type) << "." << std::endl;
  }
  vars.push_scope();
  scan_body(body);
  vars.pop_scope();
};

void Semantics::scan_for(Node *node) {
  auto for_node = static_cast<For_Node *>(node);

  vars.push_scope();

  auto init = for_node->m_init.get();
  auto cond = for_node->m_condition.get();
  // auto inc = for_node->m_incr.get();
  auto body = for_node->m_body.get();

  scan_var_decl(init);
  auto cond_type = scan_expr(cond);
  // auto inc_type = scan_expr(inc);

  if (cond_type != TokenType::bool_ && cond_type != TokenType::bool_lit &&
      cond_type != TokenType::err) {
    error(node->m_line) << "TS not a proper condition son. The middle part of "
                           "a joeFor needs a joeBool (like i < 10) but this "
                           "is a "
                        << type_name(cond_type) << "." << std::endl;
  }

  scan_body(body);
  vars.pop_scope();
};

TokenType Semantics::scan_return(Node *node) {
  auto ret = static_cast<Return_Node *>(node);

  return scan_expr(ret->m_value.get());
};

void Semantics::scan_body(Node *node) {
  auto body = static_cast<Body_Node *>(node);
  auto &items = body->m_items;

  for (std::size_t i = 0; i < items.size(); i++) {
    scan_statement(items[i].get());
  }
};

TokenType Semantics::scan_expr(Node *node) {
  auto type = node->m_type;
  if (type == Node_Type::bool_lit) {
    return TokenType::bool_lit;
  }

  else if (type == Node_Type::func_call) {
    return scan_function_call(node);
  }

  else if (type == Node_Type::int_lit) {
    return TokenType::int_lit;
  }

  else if (type == Node_Type::float_lit) {
    return TokenType::float_lit;
  }

  else if (type == Node_Type::str_lit) {
    return TokenType::str_lit;
  }

  else if (type == Node_Type::char_lit) {
    return TokenType::char_lit;
  }

  else if (type == Node_Type::var_ref) {
    return scan_var_ref(node).m_type;
  }

  else if (type == Node_Type::un_expr) {
    return scan_unary_expr(node);
  }

  else if (type == Node_Type::bin_expr) {
    auto expr = static_cast<Expression_Node *>(node);
    auto right_type = scan_expr(expr->m_right.get());
    auto left_type = scan_expr(expr->m_left.get());
    std::string op = op_name(expr->m_op);

    // One side already got roasted, don't pile on
    if (left_type == TokenType::err || right_type == TokenType::err) {
      return TokenType::err;
    }

    // Arithmetic Ops
    if (expr->m_op == TokenType::plus || expr->m_op == TokenType::minus ||
        expr->m_op == TokenType::star || expr->m_op == TokenType::slash ||
        expr->m_op == TokenType::exp) {
      if (left_type != TokenType::int_lit &&
          left_type != TokenType::float_lit && left_type != TokenType::int_ &&
          left_type != TokenType::float_) {
        error(node->m_line)
            << "The left side of '" << op << "' is a " << type_name(left_type)
            << ". You can only do math on joeInt and joeFloat monkey."
            << std::endl;
        return TokenType::err;
      }
      if (right_type != TokenType::int_lit &&
          right_type != TokenType::float_lit && right_type != TokenType::int_ &&
          right_type != TokenType::float_) {
        error(node->m_line)
            << "The right side of '" << op << "' is a " << type_name(right_type)
            << ". You can only do math on joeInt and joeFloat monkey."
            << std::endl;
        return TokenType::err;
      }
      if (left_type == TokenType::float_lit || left_type == TokenType::float_ ||
          right_type == TokenType::float_lit ||
          right_type == TokenType::float_) {
        return TokenType::float_;
      }
      return TokenType::int_;
    }

    // Comparison Ops
    else if (expr->m_op == TokenType::equal ||
             expr->m_op == TokenType::not_eq_ ||
             expr->m_op == TokenType::great ||
             expr->m_op == TokenType::great_eq ||
             expr->m_op == TokenType::less ||
             expr->m_op == TokenType::less_eq) {
      if (to_base_type(left_type) != to_base_type(right_type)) {
        error(node->m_line)
            << "Can't compare a " << type_name(left_type) << " with a "
            << type_name(right_type) << " using '" << op
            << "'. Apples and oranges gang, both sides need "
               "to be the same type."
            << std::endl;
        return TokenType::err;
      }
      return TokenType::bool_;
    }

    // Logical Ops
    else if (expr->m_op == TokenType::logical_and ||
             expr->m_op == TokenType::logical_or) {
      if (left_type != TokenType::bool_ && left_type != TokenType::bool_lit) {
        error(node->m_line)
            << "The left side of '" << op << "' is a " << type_name(left_type)
            << " but it needs to be a joeBool. Logic only works on true/false "
               "bro."
            << std::endl;
        return TokenType::err;
      }
      if (right_type != TokenType::bool_ && right_type != TokenType::bool_lit) {
        error(node->m_line)
            << "The right side of '" << op << "' is a " << type_name(right_type)
            << " but it needs to be a joeBool. Logic only works on true/false "
               "bro."
            << std::endl;
        return TokenType::err;
      }
      return TokenType::bool_;
    }

    // Assignment Ops
    else if (expr->m_op == TokenType::assign) {
      if (expr->m_left->m_type != Node_Type::var_ref) {
        error(node->m_line) << "The left side of '=' has to be a variable. You "
                               "can't assign to that, what are you doing."
                            << std::endl;
        return TokenType::err;
      }
      auto var_info = scan_var_ref(expr->m_left.get());
      if (var_info.m_type == TokenType::err) {
        return TokenType::err;
      }
      if (var_info.m_type != to_base_type(right_type)) {
        error(node->m_line)
            << "'" << var_info.m_name << "' is a " << type_name(var_info.m_type)
            << ", you can't assign a " << type_name(right_type)
            << " to it. Types don't change after you declare "
               "them chief."
            << std::endl;
        return TokenType::err;
      }
      return var_info.m_type;
    }

    // Compound Assignment Ops
    else if (expr->m_op == TokenType::plus_eq ||
             expr->m_op == TokenType::minus_eq ||
             expr->m_op == TokenType::star_eq ||
             expr->m_op == TokenType::slash_eq) {
      if (expr->m_left->m_type != Node_Type::var_ref) {
        error(node->m_line) << "The left side of '" << op
                            << "' has to be a variable. You can't assign to "
                               "that, what are you doing."
                            << std::endl;
        return TokenType::err;
      }
      auto var_info = scan_var_ref(expr->m_left.get());
      if (var_info.m_type == TokenType::err) {
        return TokenType::err;
      }
      if (var_info.m_type != TokenType::int_ &&
          var_info.m_type != TokenType::float_) {
        error(node->m_line)
            << "'" << op << "' only works on joeInt and "
            << "joeFloat, but '" << var_info.m_name << "' is a "
            << type_name(var_info.m_type) << ". Nice try tho." << std::endl;
        return TokenType::err;
      }
      if (var_info.m_type != to_base_type(right_type)) {
        error(node->m_line)
            << "'" << var_info.m_name << "' is a " << type_name(var_info.m_type)
            << " but the value "
            << "after '" << op << "' is a " << type_name(right_type)
            << ". Both sides gotta match gang." << std::endl;
        return TokenType::err;
      }
      return var_info.m_type;
    }

    else {
      error(node->m_line) << "The operator '" << op
                          << "' isn't supported yet gang. Skill issue on our "
                             "end, not yours (this time)."
                          << std::endl;
      return TokenType::err;
    }
  }

  else {
    error(node->m_line) << "Expected a value here but got a whole statement "
                           "gang. Statements can't be used as values."
                        << std::endl;
    return TokenType::err;
  }
};

TokenType Semantics::scan_unary_expr(Node *node) {
  auto expr = static_cast<Unary_Node *>(node);
  auto type = scan_expr(expr->m_right.get());
  auto op = expr->m_op;

  // Already got roasted, don't pile on
  if (type == TokenType::err) {
    return TokenType::err;
  }

  if (op == TokenType::minus) {
    if (type != TokenType::int_ && type != TokenType::int_lit &&
        type != TokenType::float_ && type != TokenType::float_lit) {
      error(node->m_line) << "You can't put a '-' in front of a "
                          << type_name(type)
                          << ". Only joeInt and joeFloat can be negative bro."
                          << std::endl;
      return TokenType::err;
    }
    return type;
  }

  else if (op == TokenType::not_) {
    if (type != TokenType::bool_ && type != TokenType::bool_lit) {
      error(node->m_line) << "'!' only works on joeBool, but this is a "
                          << type_name(type)
                          << ". You can't 'not' a number son." << std::endl;
      return TokenType::err;
    }
    return TokenType::bool_;
  }

  else if (op == TokenType::plus_plus || op == TokenType::minus_minus) {
    if (type != TokenType::int_ && type != TokenType::int_lit &&
        type != TokenType::float_ && type != TokenType::float_lit) {
      error(node->m_line) << "'" << op_name(op)
                          << "' only works on joeInt and joeFloat, but this "
                             "is a "
                          << type_name(type)
                          << ". What did you expect to happen." << std::endl;
      return TokenType::err;
    }
    return type;
  }

  else {
    error(node->m_line) << "The operator '" << op_name(op)
                        << "' can't go in front of a value gang." << std::endl;
    return TokenType::err;
  }
}

void Semantics::scan_print(Node *node) {
  auto print = static_cast<Print_Node *>(node);
  scan_expr(print->m_child.get());
};
