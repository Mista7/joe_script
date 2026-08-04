#include "codegen.h"
#include "parser.h"

std::unique_ptr<llvm::LLVMContext> CodeGen::TheContext =
    std::make_unique<llvm::LLVMContext>();

std::unique_ptr<llvm::Module> CodeGen::TheModule =
    std::make_unique<llvm::Module>("JoeScript", *TheContext);

std::unique_ptr<llvm::IRBuilder<>> CodeGen::Builder =
    std::make_unique<llvm::IRBuilder<>>(*TheContext);

std::map<std::string, llvm::Value *> CodeGen::NamedValues;

llvm::Value *CodeGen::visit(const Intlit_Node *n) {
  llvm::Type *IntTy = llvm::Type::getInt32Ty(*TheContext);
  return llvm::ConstantInt::get(IntTy, n->m_val, /*isSigned=*/true);
}

llvm::Value *CodeGen::visit(const Boollit_Node *n) {
  llvm::Type *IntTy = llvm::Type::getInt1Ty(*TheContext);
  return llvm::ConstantInt::get(IntTy, n->m_val, /*isSigned=*/true);
}

llvm::Value *CodeGen::visit(const Floatlit_Node *n) {
  llvm::Type *IntTy = llvm::Type::getDoubleTy(*TheContext);
  return llvm::ConstantFP::get(IntTy, n->m_val);
}

llvm::Value *CodeGen::visit(const Strlit_Node *n) {
  return Builder->CreateGlobalString(n->m_val);
}

llvm::Value *CodeGen::visit(const Charlit_Node *n) {
  llvm::Type *CharTy = llvm::Type::getInt8Ty(*TheContext);
  return llvm::ConstantInt::get(CharTy, static_cast<uint8_t>(n->m_val), false);
};

llvm::Value *visit(const Var_Node *n) {

};

llvm::Value *visit(const Root_Node *n) {};
llvm::Value *visit(const If_Node *n) {};
llvm::Value *visit(const Elif_Node *n) {};
llvm::Value *visit(const Else_Node *n) {};
llvm::Value *visit(const Return_Node *n) {};
llvm::Value *visit(const While_Node *n) {};
llvm::Value *visit(const For_Node *n) {};
llvm::Value *visit(const Body_Node *n) {};
llvm::Value *visit(const Print_Node *n) {};
llvm::Value *visit(const VarRef_Node *n) {};
llvm::Value *visit(const Expression_Node *n) {};
llvm::Value *visit(const Unary_Node *n) {};
