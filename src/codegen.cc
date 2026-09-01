#include "codegen.h"
#include "parser.h"

std::unique_ptr<llvm::LLVMContext> CodeGen::TheContext =
    std::make_unique<llvm::LLVMContext>();

std::unique_ptr<llvm::Module> CodeGen::TheModule =
    std::make_unique<llvm::Module>("JoeScript", *TheContext);

std::unique_ptr<llvm::IRBuilder<>> CodeGen::Builder =
    std::make_unique<llvm::IRBuilder<>>(*TheContext);

std::vector<std::map<std::string, llvm::AllocaInst *>> CodeGen::NamedValues;

void push_codegen_scope() {
  CodeGen::NamedValues.push_back(std::map<std::string, llvm::AllocaInst *>());
}

void pop_codegen_scope() { CodeGen::NamedValues.pop_back(); }

llvm::AllocaInst *get_alloca(const std::string &name) {
  for (auto it = CodeGen::NamedValues.rbegin();
       it != CodeGen::NamedValues.rend(); ++it) {
    if (it->find(name) != it->end()) {
      return it->at(name);
    }
  }
  return nullptr;
}

// Root Node IR
llvm::Value *CodeGen::visit(const Root_Node *n) {
  // Define 'int main()'
  llvm::FunctionType *FT =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), false);
  llvm::Function *MainFunc = llvm::Function::Create(
      FT, llvm::Function::ExternalLinkage, "main", TheModule.get());

  // Create the entry block and attach the builder
  llvm::BasicBlock *BB =
      llvm::BasicBlock::Create(*TheContext, "entry", MainFunc);
  Builder->SetInsertPoint(BB);

  // Initialize the global scope
  push_codegen_scope();

  // Generate IR for all top-level statements/functions
  for (const auto &child : n->m_children) {
    child->accept(this);
  }

  // Cap off the main function with 'return 0;'
  Builder->CreateRet(
      llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true)));

  pop_codegen_scope();
  return MainFunc;
}

// Var Node IR
llvm::Value *CodeGen::visit(const Var_Node *n) {
  // 1. Determine the LLVM type based on your TokenType
  llvm::Type *varType = nullptr;
  if (n->m_type == TokenType::int_ || n->m_type == TokenType::int_lit) {
    varType = llvm::Type::getInt32Ty(*TheContext);
  } else if (n->m_type == TokenType::float_ ||
             n->m_type == TokenType::float_lit) {
    varType = llvm::Type::getDoubleTy(*TheContext);
  } else if (n->m_type == TokenType::bool_ ||
             n->m_type == TokenType::bool_lit) {
    varType = llvm::Type::getInt1Ty(*TheContext);
  } else if (n->m_type == TokenType::str_ || n->m_type == TokenType::str_lit) {
    // Treat strings as opaque pointers
    varType = llvm::PointerType::getUnqual(*TheContext);
  }

  if (!varType) {
    std::cerr << "Unknown type for LLVM allocation: " << n->m_name << std::endl;
    return nullptr;
  }

  // 2. Allocate memory at the top of the current block
  llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
  llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                         TheFunction->getEntryBlock().begin());
  llvm::AllocaInst *Alloca = TmpB.CreateAlloca(varType, nullptr, n->m_name);

  // 3. Evaluate and store the assignment
  if (n->m_assigned && n->m_val) {
    llvm::Value *InitVal = n->m_val->accept(this);
    if (InitVal) {
      Builder->CreateStore(InitVal, Alloca);
    }
  }

  // 4. Register the pointer in the current lexical scope
  CodeGen::NamedValues.back()[n->m_name] = Alloca;

  return Alloca;
}

// VarRef IR
llvm::Value *CodeGen::visit(const VarRef_Node *n) {
  llvm::AllocaInst *Alloca = get_alloca(n->m_name);
  if (!Alloca) {
    std::cerr << "Unknown variable referenced in codegen: " << n->m_name
              << std::endl;
    return nullptr;
  }

  // Load the value from the stack memory pointer
  return Builder->CreateLoad(Alloca->getAllocatedType(), Alloca,
                             n->m_name.c_str());
}

// Expression_Node IR
llvm::Value *CodeGen::visit(const Expression_Node *n) {
  // Handle Assignment Separately (Left side must be a memory location, not just
  // an evaluated value)
  if (n->m_op == TokenType::assign) {
    if (auto varRef = dynamic_cast<const VarRef_Node *>(n->m_left.get())) {
      llvm::Value *rightVal = n->m_right->accept(this);
      llvm::AllocaInst *Alloca = get_alloca(varRef->m_name);
      if (!Alloca) {
        std::cerr << "Unknown variable assignment: " << varRef->m_name
                  << std::endl;
        return nullptr;
      }
      Builder->CreateStore(rightVal, Alloca);
      return rightVal;
    }
    std::cerr << "Invalid left-hand side of assignment." << std::endl;
    return nullptr;
  }

  // Evaluate sides for math/logic
  llvm::Value *L = n->m_left->accept(this);
  llvm::Value *R = n->m_right->accept(this);
  if (!L || !R)
    return nullptr;

  // Determine if we need floating-point instructions
  bool isFloat = L->getType()->isDoubleTy() || R->getType()->isDoubleTy();

  switch (n->m_op) {
  case TokenType::plus:
    return isFloat ? Builder->CreateFAdd(L, R, "addtmp")
                   : Builder->CreateAdd(L, R, "addtmp");
  case TokenType::minus:
    return isFloat ? Builder->CreateFSub(L, R, "subtmp")
                   : Builder->CreateSub(L, R, "subtmp");
  case TokenType::star:
    return isFloat ? Builder->CreateFMul(L, R, "multmp")
                   : Builder->CreateMul(L, R, "multmp");
  case TokenType::slash:
    return isFloat ? Builder->CreateFDiv(L, R, "divtmp")
                   : Builder->CreateSDiv(L, R, "divtmp");
  case TokenType::equal:
    return isFloat ? Builder->CreateFCmpUEQ(L, R, "eqtmp")
                   : Builder->CreateICmpEQ(L, R, "eqtmp");
  case TokenType::not_eq_:
    return isFloat ? Builder->CreateFCmpUNE(L, R, "netmp")
                   : Builder->CreateICmpNE(L, R, "netmp");
  case TokenType::less:
    return isFloat ? Builder->CreateFCmpULT(L, R, "lttmp")
                   : Builder->CreateICmpSLT(L, R, "lttmp");
  case TokenType::less_eq:
    return isFloat ? Builder->CreateFCmpULE(L, R, "letmp")
                   : Builder->CreateICmpSLE(L, R, "letmp");
  case TokenType::great:
    return isFloat ? Builder->CreateFCmpUGT(L, R, "gttmp")
                   : Builder->CreateICmpSGT(L, R, "gttmp");
  case TokenType::great_eq:
    return isFloat ? Builder->CreateFCmpUGE(L, R, "getmp")
                   : Builder->CreateICmpSGE(L, R, "getmp");
  default:
    std::cerr << "Invalid binary operator." << std::endl;
    return nullptr;
  }
}

// Unary IR
llvm::Value *CodeGen::visit(const Unary_Node *n) {
  llvm::Value *Operand = n->m_right->accept(this);
  if (!Operand)
    return nullptr;

  bool isFloat = Operand->getType()->isDoubleTy();

  switch (n->m_op) {
  case TokenType::minus:
    return isFloat ? Builder->CreateFNeg(Operand, "negtmp")
                   : Builder->CreateNeg(Operand, "negtmp");
  case TokenType::not_:
    return Builder->CreateNot(Operand, "nottmp");
  default:
    std::cerr << "Unsupported unary operator." << std::endl;
    return nullptr;
  }
}

// Body IR
llvm::Value *CodeGen::visit(const Body_Node *n) {
  push_codegen_scope();

  llvm::Value *lastVal = nullptr;
  for (const auto &item : n->m_items) {
    lastVal = item->accept(this);
  }

  pop_codegen_scope();
  return lastVal;
}

// Print IR
llvm::Value *CodeGen::visit(const Print_Node *n) {
  llvm::Value *val = n->m_child->accept(this);
  if (!val)
    return nullptr;

  // Retrieve or declare the printf signature: int printf(char*, ...)
  llvm::Function *printfFunc = TheModule->getFunction("printf");
  if (!printfFunc) {
    llvm::FunctionType *printfType =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext),
                                llvm::PointerType::getUnqual(*TheContext),
                                true); // variadic
    printfFunc = llvm::Function::Create(
        printfType, llvm::Function::ExternalLinkage, "printf", TheModule.get());
  }

  // Create standard formatting strings based on the passed type
  llvm::Value *formatStr = nullptr;
  if (val->getType()->isIntegerTy(32) || val->getType()->isIntegerTy(1)) {
    formatStr = Builder->CreateGlobalString("%d\n");
  } else if (val->getType()->isDoubleTy()) {
    formatStr = Builder->CreateGlobalString("%f\n");
  } else {
    formatStr = Builder->CreateGlobalString("%s\n");
  }

  return Builder->CreateCall(printfFunc, {formatStr, val});
}

// Return IR
llvm::Value *CodeGen::visit(const Return_Node *n) {
  llvm::Value *RetVal = n->m_value->accept(this);
  if (!RetVal)
    return nullptr;

  Builder->CreateRet(RetVal);
  return RetVal;
}

// If IR
llvm::Value *CodeGen::visit(const If_Node *n) {
  llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

  // Create blocks for the Then path and the Merge (after the if)
  llvm::BasicBlock *ThenBB =
      llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
  llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont");

  // Evaluate the condition (Semantic analyzer ensures this is an i1 boolean)
  llvm::Value *CondV = n->m_condition->accept(this);
  if (!CondV)
    return nullptr;

  // Branch to ThenBB if true, otherwise skip to MergeBB
  Builder->CreateCondBr(CondV, ThenBB, MergeBB);

  // -- Generate 'Then' Block --
  Builder->SetInsertPoint(ThenBB);
  n->m_body->accept(this);

  // Every block must end with a terminator. If the body didn't 'return', branch
  // to merge.
  if (!Builder->GetInsertBlock()->getTerminator()) {
    Builder->CreateBr(MergeBB);
  }

  // -- Generate 'Merge' Block --
  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);

  return nullptr; // Statements don't return an evaluated value
}

// Since Elif is parsed as a standalone node currently, its logic matches
// If_Node
llvm::Value *CodeGen::visit(const Elif_Node *n) {
  llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
  llvm::BasicBlock *ThenBB =
      llvm::BasicBlock::Create(*TheContext, "elifthen", TheFunction);
  llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*TheContext, "elifcont");

  llvm::Value *CondV = n->m_condition->accept(this);
  if (!CondV)
    return nullptr;

  Builder->CreateCondBr(CondV, ThenBB, MergeBB);

  Builder->SetInsertPoint(ThenBB);
  n->m_body->accept(this);
  if (!Builder->GetInsertBlock()->getTerminator()) {
    Builder->CreateBr(MergeBB);
  }

  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);
  return nullptr;
}

llvm::Value *CodeGen::visit(const Else_Node *n) {
  llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
  llvm::BasicBlock *ElseBB =
      llvm::BasicBlock::Create(*TheContext, "elseblock", TheFunction);
  llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*TheContext, "elsecont");

  Builder->CreateBr(ElseBB); // Unconditional jump into else

  Builder->SetInsertPoint(ElseBB);
  n->m_body->accept(this);
  if (!Builder->GetInsertBlock()->getTerminator()) {
    Builder->CreateBr(MergeBB);
  }

  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);
  return nullptr;
}

// For IR
llvm::Value *CodeGen::visit(const For_Node *n) {
  // Push a new scope so the loop initializer (e.g., int i = 0) is destroyed
  // after the loop
  push_codegen_scope();

  // 1. Execute Initializer
  if (n->m_init) {
    n->m_init->accept(this);
  }

  llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
  llvm::BasicBlock *CondBB =
      llvm::BasicBlock::Create(*TheContext, "forcond", TheFunction);
  llvm::BasicBlock *LoopBB =
      llvm::BasicBlock::Create(*TheContext, "forloop", TheFunction);
  llvm::BasicBlock *IncBB =
      llvm::BasicBlock::Create(*TheContext, "forinc", TheFunction);
  llvm::BasicBlock *AfterBB =
      llvm::BasicBlock::Create(*TheContext, "forcont", TheFunction);

  // 2. Jump to Condition
  Builder->CreateBr(CondBB);
  Builder->SetInsertPoint(CondBB);

  llvm::Value *CondV = n->m_condition->accept(this);
  if (!CondV)
    return nullptr;
  Builder->CreateCondBr(CondV, LoopBB, AfterBB);

  // 3. Generate Loop Body
  Builder->SetInsertPoint(LoopBB);
  n->m_body->accept(this);

  // Branch to the increment block instead of condition block
  if (!Builder->GetInsertBlock()->getTerminator()) {
    Builder->CreateBr(IncBB);
  }

  // 4. Generate Increment Block
  Builder->SetInsertPoint(IncBB);
  if (n->m_incr) {
    n->m_incr->accept(this);
  }
  // Loop back around to condition
  Builder->CreateBr(CondBB);

  // 5. Exit Loop
  Builder->SetInsertPoint(AfterBB);

  pop_codegen_scope();
  return nullptr;
}

// While IR
llvm::Value *CodeGen::visit(const While_Node *n) {
  llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *CondBB =
      llvm::BasicBlock::Create(*TheContext, "whilecond", TheFunction);
  llvm::BasicBlock *LoopBB =
      llvm::BasicBlock::Create(*TheContext, "whileloop", TheFunction);
  llvm::BasicBlock *AfterBB =
      llvm::BasicBlock::Create(*TheContext, "whilecont", TheFunction);

  // Jump from current flow into the condition block
  Builder->CreateBr(CondBB);
  Builder->SetInsertPoint(CondBB);

  // Evaluate condition and branch to loop or exit
  llvm::Value *CondV = n->m_condition->accept(this);
  if (!CondV)
    return nullptr;
  Builder->CreateCondBr(CondV, LoopBB, AfterBB);

  // Generate the loop body
  Builder->SetInsertPoint(LoopBB);
  n->m_body->accept(this);

  // Jump back to the condition to restart the loop
  if (!Builder->GetInsertBlock()->getTerminator()) {
    Builder->CreateBr(CondBB);
  }

  // Set insertion point to continue the rest of the program
  Builder->SetInsertPoint(AfterBB);
  return nullptr;
}

// Helper function to map TokenType to llvm::Type
llvm::Type *get_llvm_type(TokenType type) {
  if (type == TokenType::int_ || type == TokenType::int_lit) {
    return llvm::Type::getInt32Ty(*CodeGen::TheContext);
  } else if (type == TokenType::float_ || type == TokenType::float_lit) {
    return llvm::Type::getDoubleTy(*CodeGen::TheContext);
  } else if (type == TokenType::bool_ || type == TokenType::bool_lit) {
    return llvm::Type::getInt1Ty(*CodeGen::TheContext);
  } else if (type == TokenType::char_ || type == TokenType::char_lit) {
    return llvm::Type::getInt8Ty(*CodeGen::TheContext);
  } else if (type == TokenType::str_ || type == TokenType::str_lit) {
    // Treat strings as opaque pointers in LLVM
    return llvm::PointerType::getUnqual(*CodeGen::TheContext);
  }
  // Default fallback
  return llvm::Type::getInt32Ty(*CodeGen::TheContext);
}

llvm::Value *CodeGen::visit(const FunctionDecl_Node *n) {
  // 1. Map the parameter types
  std::vector<llvm::Type *> paramTypes;
  for (const auto &param : n->m_params) {
    paramTypes.push_back(get_llvm_type(param.first));
  }

  // 2. Map the return type
  llvm::Type *retType = get_llvm_type(n->m_return_type);

  // 3. Create the Function Type and the Function itself
  llvm::FunctionType *FT = llvm::FunctionType::get(retType, paramTypes, false);
  llvm::Function *F = llvm::Function::Create(
      FT, llvm::Function::ExternalLinkage, n->m_name, TheModule.get());

  // 4. Create the entry block for the function
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);

  // Save current insertion block to restore it later
  llvm::BasicBlock *OldBB = Builder->GetInsertBlock();
  Builder->SetInsertPoint(BB);

  // 5. Push a new scope for the function body
  push_codegen_scope();

  // 6. Name the arguments and allocate them on the stack
  unsigned Idx = 0;
  for (auto &Arg : F->args()) {
    std::string paramName = n->m_params[Idx++].second;
    Arg.setName(paramName);

    // Allocate stack memory for the argument
    llvm::AllocaInst *Alloca =
        Builder->CreateAlloca(Arg.getType(), nullptr, paramName);

    // Store the initial incoming value into the stack memory
    Builder->CreateStore(&Arg, Alloca);

    // Register the pointer in the current scope so VarRef can find it
    CodeGen::NamedValues.back()[paramName] = Alloca;
  }

  // 7. Generate the function body
  n->m_body->accept(this);

  // 8. Ensure the function has a return statement if the user forgot one
  if (!Builder->GetInsertBlock()->getTerminator()) {
    if (retType->isVoidTy()) {
      Builder->CreateRetVoid();
    } else {
      // Return a default 0 if they forgot
      Builder->CreateRet(llvm::Constant::getNullValue(retType));
    }
  }

  // 9. Clean up and restore the builder
  pop_codegen_scope();
  Builder->SetInsertPoint(OldBB);

  return F;
}

llvm::Value *CodeGen::visit(const FunctionCall_Node *n) {
  // 1. Look up the name in the global module table
  llvm::Function *CalleeF = TheModule->getFunction(n->m_name);
  if (!CalleeF) {
    std::cerr << "Unknown function referenced: " << n->m_name << std::endl;
    return nullptr;
  }

  // 2. Verify argument count matches
  if (CalleeF->arg_size() != n->m_args.size()) {
    std::cerr << "Incorrect number of arguments passed to " << n->m_name
              << std::endl;
    return nullptr;
  }

  // 3. Evaluate the arguments
  std::vector<llvm::Value *> ArgsV;
  for (unsigned i = 0, e = n->m_args.size(); i != e; ++i) {
    llvm::Value *argVal = n->m_args[i]->accept(this);
    if (!argVal) {
      return nullptr;
    }
    ArgsV.push_back(argVal);
  }

  // 4. Generate the Call instruction
  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

// Int IR
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
