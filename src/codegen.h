#pragma once
#include "parser.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Visitor {
public:
  virtual ~Visitor() = default;

  virtual llvm::Value *visit(const Root_Node *n) = 0;
  virtual llvm::Value *visit(const If_Node *n) = 0;
  virtual llvm::Value *visit(const Elif_Node *n) = 0;
  virtual llvm::Value *visit(const Else_Node *n) = 0;
  virtual llvm::Value *visit(const Return_Node *n) = 0;
  virtual llvm::Value *visit(const While_Node *n) = 0;
  virtual llvm::Value *visit(const For_Node *n) = 0;
  virtual llvm::Value *visit(const Body_Node *n) = 0;
  virtual llvm::Value *visit(const Print_Node *n) = 0;
  virtual llvm::Value *visit(const Strlit_Node *n) = 0;
  virtual llvm::Value *visit(const Intlit_Node *n) = 0;
  virtual llvm::Value *visit(const Boollit_Node *n) = 0;
  virtual llvm::Value *visit(const Charlit_Node *n) = 0;
  virtual llvm::Value *visit(const Floatlit_Node *n) = 0;
  virtual llvm::Value *visit(const Var_Node *n) = 0;
  virtual llvm::Value *visit(const VarRef_Node *n) = 0;
  virtual llvm::Value *visit(const Expression_Node *n) = 0;
  virtual llvm::Value *visit(const Unary_Node *n) = 0;
  virtual llvm::Value *visit(const FunctionDecl_Node *n) = 0;
  virtual llvm::Value *visit(const FunctionCall_Node *n) = 0;
};

class CodeGen : public Visitor {
public:
  static std::unique_ptr<llvm::LLVMContext> TheContext;
  static std::unique_ptr<llvm::Module> TheModule;
  static std::unique_ptr<llvm::IRBuilder<>> Builder;
  static std::vector<std::map<std::string, llvm::AllocaInst *>> NamedValues;

  virtual llvm::Value *visit(const Root_Node *n) override;
  virtual llvm::Value *visit(const If_Node *n) override;
  virtual llvm::Value *visit(const Elif_Node *n) override;
  virtual llvm::Value *visit(const Else_Node *n) override;
  virtual llvm::Value *visit(const Return_Node *n) override;
  virtual llvm::Value *visit(const While_Node *n) override;
  virtual llvm::Value *visit(const For_Node *n) override;
  virtual llvm::Value *visit(const Body_Node *n) override;
  virtual llvm::Value *visit(const Print_Node *n) override;
  virtual llvm::Value *visit(const Strlit_Node *n) override;
  virtual llvm::Value *visit(const Intlit_Node *n) override;
  virtual llvm::Value *visit(const Boollit_Node *n) override;
  virtual llvm::Value *visit(const Charlit_Node *n) override;
  virtual llvm::Value *visit(const Floatlit_Node *n) override;
  virtual llvm::Value *visit(const Var_Node *n) override;
  virtual llvm::Value *visit(const VarRef_Node *n) override;
  virtual llvm::Value *visit(const Expression_Node *n) override;
  virtual llvm::Value *visit(const Unary_Node *n) override;
  virtual llvm::Value *visit(const FunctionDecl_Node *n) override;
  virtual llvm::Value *visit(const FunctionCall_Node *n) override;
};

// @ => functions, global variables
// % => Local identifiers (register names, types)
//
