#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "semantics.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

#include <cctype>
#include <cstdlib> // For std::system
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// How to compile:
// clang++ -g -O3 *.cc `llvm-config --cxxflags --ldflags --system-libs --libs
// core native` -o joec
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
  case Node_Type::func_decl:
    std::cout << indent << "FuncDecl: "
              << static_cast<const FunctionDecl_Node *>(node)->m_name
              << std::endl;
    print_node(static_cast<const FunctionDecl_Node *>(node)->m_body.get(),
               depth + 1);
    break;
  case Node_Type::func_call:
    std::cout << indent << "FuncCall: "
              << static_cast<const FunctionCall_Node *>(node)->m_name
              << std::endl;
    for (const auto &arg :
         static_cast<const FunctionCall_Node *>(node)->m_args) {
      print_node(arg.get(), depth + 1);
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

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename.joe>\n";
    return 1;
  }

  std::ifstream inputfile(argv[1]);

  if (!inputfile.is_open()) {
    std::cout << "ERROR: Could not open " << argv[1] << "\n";
    return 1;
  }

  std::string line;
  std::string file;

  while (std::getline(inputfile, line)) {
    file += line;
    file += '\n';
  }

  // Lexical Analysis
  Tokenizer tokenize_file(file);
  std::vector<Token> tokens = tokenize_file.tokenize();

  // Parse into AST
  Parser ast(tokens);
  std::unique_ptr<Root_Node> root = ast.parser();

  // Print AST
  std::cout << "--- Abstract Syntax Tree ---\n";
  print_node(root.get());

  // Extract raw pointer because Semantics takes unique_ptr ownership via
  // std::move
  Root_Node *raw_root = root.get();

  // Run semantic analysis
  Semantics sem(std::move(root));
  sem.scan();
  std::cout << "\nSemantic analysis complete.\n";

  // Run IR Code Generation
  CodeGen codegen;
  codegen.visit(raw_root);

  std::cout << "\n--- Generated LLVM IR ---\n";
  CodeGen::TheModule->print(llvm::errs(), nullptr);

  // --- Object File Emission ---
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  std::string TargetTripleStr = llvm::sys::getDefaultTargetTriple();
  llvm::Triple TargetTriple(
      TargetTripleStr); // Explicitly create the Triple object
  CodeGen::TheModule->setTargetTriple(TargetTriple);

  std::string Error;
  // Pass the explicit Triple object to lookupTarget
  auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
  if (!Target) {
    llvm::errs() << Error;
    return 1;
  }

  auto CPU = "generic";
  auto Features = "";
  llvm::TargetOptions opt;
  // Pass the explicit Triple object to createTargetMachine
  auto TheTargetMachine = Target->createTargetMachine(
      TargetTriple, CPU, Features, opt, llvm::Reloc::PIC_);
  CodeGen::TheModule->setDataLayout(TheTargetMachine->createDataLayout());

  auto Filename = "output.o";
  std::error_code EC;
  llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);
  if (EC) {
    llvm::errs() << "Could not open file: " << EC.message();
    return 1;
  }

  llvm::legacy::PassManager pass;
  auto FileType =
      llvm::CodeGenFileType::ObjectFile; // Use llvm::CGFT_ObjectFile for older
                                         // LLVM versions

  if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    llvm::errs() << "TheTargetMachine can't emit a file of this type";
    return 1;
  }

  pass.run(*CodeGen::TheModule);
  dest.flush();

  std::cout << "\nObject file compiled to " << Filename << "\n";

  // Automate the linking step using the system's C compiler
  std::cout << "Linking executable...\n";
  int linkResult = std::system("clang output.o -o a.out");

  if (linkResult == 0) {
    std::cout << "Successfully linked! Run your program with ./a.out\n";
  } else {
    std::cerr << "Linking failed. Ensure clang or gcc is installed and "
                 "available in your PATH.\n";
  }

  return 0;
}
