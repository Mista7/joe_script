#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

void print_node(const Node *node, int depth = 0) {
  if (node == nullptr) {
    return;
  }
  std::string indent(depth * 2, ' ');
  switch (node->m_type) {
  case Node_Type::root:
    std::cout << indent << "Root" << std::endl;
    for (const auto &child : static_cast<const Root_Node *>(node)->m_children) {
      print_node(child.get(), depth + 1);
    }
    break;
  case Node_Type::ret:
    std::cout << indent << "Return" << std::endl;
    print_node(static_cast<const Return_Node *>(node)->m_value.get(),
               depth + 1);
    break;
  case Node_Type::int_lit:
    std::cout << indent
              << "IntLit: " << static_cast<const Intlit_Node *>(node)->m_val
              << std::endl;
    break;
  case Node_Type::float_lit:
    std::cout << indent
              << "FloatLit: " << static_cast<const Floatlit_Node *>(node)->m_val
              << std::endl;
    break;
  case Node_Type::str_lit:
    std::cout << indent
              << "StrLit: " << static_cast<const Strlit_Node *>(node)->m_val
              << std::endl;
    break;
  case Node_Type::bool_lit:
    std::cout << indent
              << "BoolLit: " << static_cast<const Boollit_Node *>(node)->m_val
              << std::endl;
    break;
  case Node_Type::char_lit:
    std::cout << indent
              << "CharLit: " << static_cast<const Charlit_Node *>(node)->m_val
              << std::endl;
    break;
  case Node_Type::print_:
    std::cout << indent << "Print" << std::endl;
    print_node(static_cast<const Print_Node *>(node)->m_child.get(), depth + 1);
    break;
  case Node_Type::bin_expr:
    std::cout << indent << "BinaryExpr" << std::endl;
    print_node(static_cast<const Expression_Node *>(node)->m_left.get(),
               depth + 1);
    print_node(static_cast<const Expression_Node *>(node)->m_right.get(),
               depth + 1);
    break;
  case Node_Type::un_expr:
    std::cout << indent << "UnaryExpr" << std::endl;
    print_node(static_cast<const Unary_Node *>(node)->m_right.get(), depth + 1);
    break;
  case Node_Type::var:
    std::cout << indent
              << "VarDecl: " << static_cast<const Var_Node *>(node)->m_name
              << std::endl;
    print_node(static_cast<const Var_Node *>(node)->m_val.get(), depth + 1);
    break;
  case Node_Type::var_ref:
    std::cout << indent
              << "VarRef: " << static_cast<const VarRef_Node *>(node)->m_name
              << std::endl;
    break;
  case Node_Type::if_:
    std::cout << indent << "If" << std::endl;
    print_node(static_cast<const If_Node *>(node)->m_condition.get(),
               depth + 1);
    print_node(static_cast<const If_Node *>(node)->m_body.get(), depth + 1);
    break;
  case Node_Type::elif_:
    std::cout << indent << "Elif" << std::endl;
    print_node(static_cast<const Elif_Node *>(node)->m_condition.get(),
               depth + 1);
    print_node(static_cast<const Elif_Node *>(node)->m_body.get(), depth + 1);
    break;
  case Node_Type::else_:
    std::cout << indent << "Else" << std::endl;
    print_node(static_cast<const Else_Node *>(node)->m_body.get(), depth + 1);
    break;
  case Node_Type::while_:
    std::cout << indent << "While" << std::endl;
    print_node(static_cast<const While_Node *>(node)->m_condition.get(),
               depth + 1);
    print_node(static_cast<const While_Node *>(node)->m_body.get(), depth + 1);
    break;
  case Node_Type::for_:
    std::cout << indent << "For" << std::endl;
    print_node(static_cast<const For_Node *>(node)->m_init.get(), depth + 1);
    print_node(static_cast<const For_Node *>(node)->m_condition.get(),
               depth + 1);
    print_node(static_cast<const For_Node *>(node)->m_incr.get(), depth + 1);
    print_node(static_cast<const For_Node *>(node)->m_body.get(), depth + 1);
    break;
  case Node_Type::body:
    std::cout << indent << "Body" << std::endl;
    for (const auto &item : static_cast<const Body_Node *>(node)->m_items) {
      print_node(item.get(), depth + 1);
    }
    break;
  default:
    std::cout << indent << "Unknown node" << std::endl;
    break;
  }
}

int main() {
  std::ifstream inputfile("test.txt");

  if (!inputfile.is_open()) {
    std::cout << "ERROR";
    return 1;
  }

  std::string line;
  std::string file;

  while (std::getline(inputfile, line)) {
    file += line;
    file += '\n';
  }

  Tokenizer tokenize_file(file);
  std::vector<Token> tokens = tokenize_file.tokenize();

  for (auto &token : tokens) {
    // std::cout << "Value: " << token.value.value_or("no value")
    //           << " Line: " << token.line << std::endl;
    //
    std::cout << "Type: " << static_cast<int>(token.type)
              << " Value: " << token.value.value_or("no value")
              << " Line: " << token.line << std::endl;
  }

  Parser ast(tokens);
  std::unique_ptr<Root_Node> root = ast.parser();

  // Print AST
  print_node(root.get());

  // Run semantic analysis
  Semantics sem(std::move(root));
  sem.scan();

  std::cout << "\nSemantic analysis complete.\n";

  return 0;

  return 0;
}
