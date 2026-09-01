#include "semantics.h"
#include "lexer.h"
#include "parser.h"
#include "symbol-table.h"
#include <cstddef>
#include <iostream>

TokenType to_base_type(TokenType type);
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
    std::cerr << "Function already declared: " << func->m_name << std::endl;
    return;
  }
  vars.add_func(func->m_name, info);

  // 2. Push a new scope for the function body
  vars.push_scope();

  // 3. Add parameters to the local scope as assigned variables
  for (const auto &param : func->m_params) {
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
    std::cerr << "Function not declared: " << call->m_name << std::endl;
    return TokenType::err;
  }

  Func_Info info = opt_func.value();

  if (info.param_types.size() != call->m_args.size()) {
    std::cerr << "Argument count mismatch for " << call->m_name << ". Expected "
              << info.param_types.size() << ", got " << call->m_args.size()
              << std::endl;
    return TokenType::err;
  }

  for (size_t i = 0; i < call->m_args.size(); i++) {
    TokenType arg_type = scan_expr(call->m_args[i].get());
    if (to_base_type(arg_type) != to_base_type(info.param_types[i])) {
      std::cerr << "Type mismatch in argument " << i << " for function "
                << call->m_name << std::endl;
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

  else if (type == Node_Type::else_) {
    scan_else(child);
  }

  else {
    std::cerr << "Unknown Node son" << std::endl;
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
    std::cerr << "Variable has already been declared " << name << std::endl;
  } else {
    if (!assigned || (var->m_assigned &&
                      to_base_type(scan_expr(var->m_val.get())) == type)) {
      auto info = Var_Info(type, assigned, name);
      vars.add_local(info);
    } else {
      std::cerr << "Invalid type for this variable: " << name << std::endl;
    }
  }
};

Var_Info Semantics::scan_var_ref(Node *node) {
  auto ref = static_cast<VarRef_Node *>(node);
  auto info = vars.search_var(ref->m_name);

  if (!info.has_value()) {
    std::cerr << "This variable hasn't been declared yet: " << ref->m_name
              << std::endl;
    return Var_Info();
  }

  return info.value();
};

void Semantics::scan_if(Node *node) {
  auto if_node = static_cast<If_Node *>(node);
  auto cond = if_node->m_condition.get();
  auto body = if_node->m_body.get();

  TokenType cond_type = scan_expr(cond);
  if (cond_type != TokenType::bool_ && cond_type != TokenType::bool_lit) {
    std::cerr << "TS not a proper condition son" << std::endl;
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
  if (cond_type != TokenType::bool_ && cond_type != TokenType::bool_lit) {
    std::cerr << "TS not a proper condition son" << std::endl;
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

  if (cond_type != TokenType::bool_ && cond_type != TokenType::bool_lit) {
    std::cerr << "TS not a proper condition son" << std::endl;
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

    // Arithmetic Ops
    if (expr->m_op == TokenType::plus || expr->m_op == TokenType::minus ||
        expr->m_op == TokenType::star || expr->m_op == TokenType::slash ||
        expr->m_op == TokenType::exp) {
      if (left_type != TokenType::int_lit &&
          left_type != TokenType::float_lit && left_type != TokenType::int_ &&
          left_type != TokenType::float_) {
        std::cerr << "Left side must be numeric" << std::endl;
        return TokenType::err;
      }
      if (right_type != TokenType::int_lit &&
          right_type != TokenType::float_lit && right_type != TokenType::int_ &&
          right_type != TokenType::float_) {
        std::cerr << "Right side must be numeric" << std::endl;
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
        std::cerr << "Cannot compare different types" << std::endl;
        return TokenType::err;
      }
      return TokenType::bool_;
    }

    // Logical Ops
    else if (expr->m_op == TokenType::logical_and ||
             expr->m_op == TokenType::logical_or) {
      if (left_type != TokenType::bool_ && left_type != TokenType::bool_lit) {
        std::cerr << "Left side must be boolean" << std::endl;
        return TokenType::err;
      }
      if (right_type != TokenType::bool_ && right_type != TokenType::bool_lit) {
        std::cerr << "Right side must be boolean" << std::endl;
        return TokenType::err;
      }
      return TokenType::bool_;
    }

    // Assignment Ops
    else if (expr->m_op == TokenType::assign) {
      if (expr->m_left->m_type != Node_Type::var_ref) {
        std::cerr << "Can only assign to a variable" << std::endl;
        return TokenType::err;
      }
      auto var_info = scan_var_ref(expr->m_left.get());
      if (var_info.m_type != to_base_type(right_type)) {
        std::cerr << "Type mismatch in assignment" << std::endl;
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
        std::cerr << "Can only assign to a variable" << std::endl;
        return TokenType::err;
      }
      auto var_info = scan_var_ref(expr->m_left.get());
      if (var_info.m_type != TokenType::int_ &&
          var_info.m_type != TokenType::float_) {
        std::cerr << "Compound assignment only works on numeric types"
                  << std::endl;
        return TokenType::err;
      }
      if (var_info.m_type != to_base_type(right_type)) {
        std::cerr << "Type mismatch in compound assignment" << std::endl;
        return TokenType::err;
      }
      return var_info.m_type;
    }

    else {
      std::cerr << "Unknown operator gang" << std::endl;
      return TokenType::err;
    }
  }

  else {
    std::cerr << "Unknown node type gang" << std::endl;
    return TokenType::err;
  }
};

TokenType Semantics::scan_unary_expr(Node *node) {
  auto expr = static_cast<Unary_Node *>(node);
  auto type = scan_expr(expr->m_right.get());
  auto op = expr->m_op;

  if (op == TokenType::minus) {
    if (type != TokenType::int_ && type != TokenType::int_lit &&
        type != TokenType::float_ && type != TokenType::float_lit) {
      std::cerr << "Unary minus only works on numeric types" << std::endl;
      return TokenType::err;
    }
    return type;
  }

  else if (op == TokenType::not_) {
    if (type != TokenType::bool_ && type != TokenType::bool_lit) {
      std::cerr << "Not operator only works on boolean types" << std::endl;
      return TokenType::err;
    }
    return TokenType::bool_;
  }

  else if (op == TokenType::plus_plus || op == TokenType::minus_minus) {
    if (type != TokenType::int_ && type != TokenType::int_lit &&
        type != TokenType::float_ && type != TokenType::float_lit) {
      std::cerr << "Increment/decrement only works on numeric types"
                << std::endl;
      return TokenType::err;
    }
    return type;
  }

  else {
    std::cerr << "Unknown unary operator" << std::endl;
    return TokenType::err;
  }
}

void Semantics::scan_print(Node *node) {
  auto print = static_cast<Print_Node *>(node);
  scan_expr(print->m_child.get());
};
