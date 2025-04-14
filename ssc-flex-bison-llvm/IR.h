#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <string>
#include <vector>

using namespace llvm;

// forward declarations
extern void yyerror(const char *err);

// struct types for control flow blocks
struct IfElseBlock {
    BasicBlock *thenBlock;
    BasicBlock *elseBlock;
    BasicBlock *mergeBlock;
    Value *condition;
};

struct ForLoopBlock {
    BasicBlock *condBlock;
    BasicBlock *bodyBlock;
    BasicBlock *incBlock;
    BasicBlock *afterBlock;
    Value *iterVar;
    Value *limitVal;
    Value *originalVar;  // store the original variable pointer
};

struct FunctionBlock {
    Function *function;
    BasicBlock *entryBlock;
    BasicBlock *returnBlock;
    BasicBlock *savedBlock;   // save the previous insertion block
    Function *savedFunction;  // save the previous function
};

// global variables
static LLVMContext *context = nullptr;
static Module *module = nullptr;
static IRBuilder<> *builder = nullptr;
static Function *mainFunction = nullptr;
static std::map<std::string, Value *> SymbolTable;
static std::map<std::string, Function *> FunctionTable;
// blocks for nested statements
static std::vector<BasicBlock *> BlockStack;

// function definitions
Value *getFromSymbolTable(const char *id);
void setDouble(const char *id, void *value);
void printString(const char *str);
void printDouble(void *value);
void *performBinaryOperation(void *lhs, void *rhs, int op);
void *performComparison(void *lhs, void *rhs, int op);
void initLLVM();
void printLLVMIR();
void addReturnInstr();
void *createDoubleConstant(double val);
void *getValueFromSymbolTable(const char *id);
void *createIfBlock(void *expr);
void completeIfBlock(void *block);
void *createIfElseBlock(void *expr);
void completeIfThen(void *block);
void completeIfElse(void *block);
void *createForLoop(const char *id, int limit);
void completeForBody(void *block);
void *createFunction(const char *name);
void completeFunction(void *block);
void callFunction(const char *name);

/**
 * init LLVM
 * Create main function (similar to C-main) that returns a int but takes no parameters.
 */
void initLLVM() {
    // Check if already initialized
    if (context != nullptr) {
        fprintf(stderr, "LLVM already initialized, skipping\n");
        return;
    }

    // Initialize LLVM components
    context = new LLVMContext();
    module = new Module("top", *context);
    builder = new IRBuilder<>(*context);

    // returns an int and has fixed number of parameters. Do not take any parameters.
    FunctionType *mainTy = FunctionType::get(builder->getInt32Ty(), false);

    // the main function definition.
    mainFunction = Function::Create(mainTy, Function::ExternalLinkage, "main", module);

    // Create entry basic block of the main function.
    BasicBlock *entry = BasicBlock::Create(*context, "entry", mainFunction);

    // Tell builder that instruction to be added in this basic block.
    builder->SetInsertPoint(entry);

    // Clear symbol tables in case of multiple initializations
    SymbolTable.clear();
    FunctionTable.clear();
    BlockStack.clear();
}

void addReturnInstr() {
    if (!builder || !context) {
        yyerror("LLVM not initialized");
        return;
    }

    builder->CreateRet(ConstantInt::get(*context, APInt(32, 0)));
}

void *createDoubleConstant(double val) {
    if (!builder || !context) {
        yyerror("LLVM not initialized");
        return nullptr;
    }

    return ConstantFP::get(*context, APFloat(val));
}

void printLLVMIR() {
    if (!module) {
        yyerror("Module not initialized");
        return;
    }

    module->print(errs(), nullptr);
}

Value *getFromSymbolTable(const char *id) {
    if (!id) {
        yyerror("Null identifier passed to getFromSymbolTable");
        return nullptr;
    }

    if (!builder || !context) {
        yyerror("LLVM not initialized");
        return nullptr;
    }

    std::string name(id);
    if (SymbolTable.find(name) != SymbolTable.end()) {
        return SymbolTable[name];
    } else {
        Value *defaultValue = builder->CreateAlloca(builder->getDoubleTy(), nullptr, name);
        builder->CreateStore(ConstantFP::get(*context, APFloat(0.0)), defaultValue);
        SymbolTable[name] = defaultValue;
        return defaultValue;
    }
}

// Get the value (not pointer) from symbol table
void *getValueFromSymbolTable(const char *id) {
    if (!id) {
        yyerror("Null identifier passed to getValueFromSymbolTable");
        return nullptr;
    }

    Value *ptr = getFromSymbolTable(id);
    if (!ptr) return nullptr;

    return builder->CreateLoad(builder->getDoubleTy(), ptr, "loadtmp");
}

void setDouble(const char *id, void *value) {
    if (!id || !value) {
        yyerror("Null parameter passed to setDouble");
        return;
    }

    Value *ptr = getFromSymbolTable(id);
    if (!ptr) return;

    builder->CreateStore((Value *)value, ptr);
}

// Print functions updated to check for null pointers
void printfLLVM(const char *format, Value *inputValue) {
    if (!format || !inputValue || !builder || !context || !module) {
        yyerror("Invalid parameters or uninitialized LLVM");
        return;
    }

    Function *printfFunc = module->getFunction("printf");
    if (!printfFunc) {
        FunctionType *printfTy = FunctionType::get(builder->getInt32Ty(), PointerType::get(builder->getInt8Ty(), 0), true);
        printfFunc = Function::Create(printfTy, Function::ExternalLinkage, "printf", module);
    }

    Value *formatVal = builder->CreateGlobalString(format);
    builder->CreateCall(printfFunc, {formatVal, inputValue}, "printfCall");
}

void printString(const char *str) {
    if (!str || !builder) {
        yyerror("Invalid parameters or uninitialized LLVM");
        return;
    }

    Value *strValue = builder->CreateGlobalString(str);
    printfLLVM("%s\n", strValue);
}

void printDouble(void *value) {
    if (!value) {
        yyerror("Null value passed to printDouble");
        return;
    }

    printfLLVM("%f\n", (Value *)value);
}

void *performBinaryOperation(void *lhs, void *rhs, int op) {
    if (!lhs || !rhs || !builder) {
        yyerror("Invalid parameters or uninitialized LLVM");
        return nullptr;
    }

    Value *lhsValue = (Value *)lhs;
    Value *rhsValue = (Value *)rhs;

    switch (op) {
        case '+':
            return builder->CreateFAdd(lhsValue, rhsValue, "fadd");
        case '-':
            return builder->CreateFSub(lhsValue, rhsValue, "fsub");
        case '*':
            return builder->CreateFMul(lhsValue, rhsValue, "fmul");
        case '/':
            return builder->CreateFDiv(lhsValue, rhsValue, "fdiv");
        default:
            yyerror("illegal binary operation");
            exit(EXIT_FAILURE);
    }
}

// Implement comparison operations for conditionals
void *performComparison(void *lhs, void *rhs, int op) {
    if (!lhs || !rhs || !builder) {
        yyerror("Invalid parameters or uninitialized LLVM");
        return nullptr;
    }

    Value *lhsValue = (Value *)lhs;
    Value *rhsValue = (Value *)rhs;

    switch (op) {
        case 1:  // ==
            return builder->CreateFCmpOEQ(lhsValue, rhsValue, "fcmp_eq");
        case 2:  // !=
            return builder->CreateFCmpONE(lhsValue, rhsValue, "fcmp_neq");
        case 3:  // <=
            return builder->CreateFCmpOLE(lhsValue, rhsValue, "fcmp_leq");
        case 4:  // >=
            return builder->CreateFCmpOGE(lhsValue, rhsValue, "fcmp_geq");
        case 5:  // <
            return builder->CreateFCmpOLT(lhsValue, rhsValue, "fcmp_lt");
        case 6:  // >
            return builder->CreateFCmpOGT(lhsValue, rhsValue, "fcmp_gt");
        default:
            yyerror("invalid comparison operator");
            exit(EXIT_FAILURE);
    }
}

// For if statements - conditionals
Value *createCondition(Value *expr) {
    if (!expr || !builder) {
        yyerror("Invalid parameters or uninitialized LLVM");
        return nullptr;
    }

    // If expr is already a condition (i1 type), return it
    if (expr->getType()->isIntegerTy(1))
        return expr;

    // Compare with zero (any non-zero value is true)
    Value *zero = (Value *)createDoubleConstant(0.0);
    return builder->CreateFCmpONE(expr, zero, "ifcond");
}

// Create if statement
void *createIfBlock(void *expr) {
    if (!expr) {
        yyerror("Null expression in if condition");
        return nullptr;
    }

    Value *condition = createCondition((Value *)expr);
    if (!condition) {
        yyerror("Failed to create condition");
        return nullptr;
    }

    IfElseBlock *ifBlock = new IfElseBlock();
    if (!ifBlock) {
        yyerror("Failed to allocate memory for if block");
        return nullptr;
    }

    ifBlock->condition = condition;

    // create basic blocks for the then case and merge point
    ifBlock->thenBlock = BasicBlock::Create(*context, "then", mainFunction);
    ifBlock->mergeBlock = BasicBlock::Create(*context, "ifcont", mainFunction);

    // create conditional branch
    builder->CreateCondBr(ifBlock->condition, ifBlock->thenBlock, ifBlock->mergeBlock);

    // start insertion in the then block
    builder->SetInsertPoint(ifBlock->thenBlock);

    return ifBlock;
}

void completeIfBlock(void *block) {
    if (!block) {
        yyerror("Null block in completeIfBlock");
        return;
    }

    IfElseBlock *ifBlock = (IfElseBlock *)block;

    // create branch from then to merge
    builder->CreateBr(ifBlock->mergeBlock);

    // continue with merge block
    builder->SetInsertPoint(ifBlock->mergeBlock);

    delete ifBlock;
}

void *createIfElseBlock(void *expr) {
    if (!expr || !builder || !context) {
        yyerror("Invalid parameters or uninitialized LLVM");
        return nullptr;
    }

    Value *condition = createCondition((Value *)expr);
    if (!condition) {
        yyerror("Failed to create condition for if-else");
        return nullptr;
    }

    IfElseBlock *ifElseBlock = new IfElseBlock();
    if (!ifElseBlock) {
        yyerror("Failed to allocate memory for if-else block");
        return nullptr;
    }

    ifElseBlock->condition = condition;

    // create basic blocks first before any branches
    ifElseBlock->thenBlock = BasicBlock::Create(*context, "then", mainFunction);
    ifElseBlock->elseBlock = BasicBlock::Create(*context, "else", mainFunction);
    ifElseBlock->mergeBlock = BasicBlock::Create(*context, "ifcont", mainFunction);

    // Create the branch instruction at the current point - entry block
    builder->CreateCondBr(ifElseBlock->condition, ifElseBlock->thenBlock, ifElseBlock->elseBlock);

    // set insertion to then block for further statements
    builder->SetInsertPoint(ifElseBlock->thenBlock);

    return ifElseBlock;
}

void completeIfThen(void *block) {
    if (!block) {
        yyerror("Null block in completeIfThen");
        return;
    }

    IfElseBlock *ifElseBlock = (IfElseBlock *)block;

    // create branch from then to merge
    builder->CreateBr(ifElseBlock->mergeBlock);

    // start insertion in else block
    builder->SetInsertPoint(ifElseBlock->elseBlock);
}

void completeIfElse(void *block) {
    if (!block) {
        yyerror("Null block in completeIfElse");
        return;
    }

    IfElseBlock *ifElseBlock = (IfElseBlock *)block;

    // create branch from else to merge
    builder->CreateBr(ifElseBlock->mergeBlock);

    // continue with merge block
    builder->SetInsertPoint(ifElseBlock->mergeBlock);

    delete ifElseBlock;
}

// create a for loop
void *createForLoop(const char *id, int limit) {
    if (!id) {
        yyerror("Null identifier in for loop");
        return nullptr;
    }

    ForLoopBlock *forLoop = new ForLoopBlock();
    if (!forLoop) {
        yyerror("Failed to allocate memory for for loop");
        return nullptr;
    }

    // get the loop variable
    Value *loopVar = getFromSymbolTable(id);
    if (!loopVar) {
        yyerror("Failed to get loop variable");
        delete forLoop;
        return nullptr;
    }

    // save the original variable pointer in the ForLoopBlock
    forLoop->originalVar = loopVar;

    // init loop variable to 0
    Value *initVal = (Value *)createDoubleConstant(0.0);
    builder->CreateStore(initVal, loopVar);

    // basic blocks for loop
    forLoop->condBlock = BasicBlock::Create(*context, "for.cond", mainFunction);
    forLoop->bodyBlock = BasicBlock::Create(*context, "for.body", mainFunction);
    forLoop->incBlock = BasicBlock::Create(*context, "for.inc", mainFunction);
    forLoop->afterBlock = BasicBlock::Create(*context, "for.end", mainFunction);

    // branch to condition block
    builder->CreateBr(forLoop->condBlock);

    // build condition check
    builder->SetInsertPoint(forLoop->condBlock);
    forLoop->iterVar = builder->CreateLoad(builder->getDoubleTy(), loopVar, "i");
    forLoop->limitVal = (Value *)createDoubleConstant(static_cast<double>(limit));
    Value *cond = builder->CreateFCmpOLT(forLoop->iterVar, forLoop->limitVal, "for.cond");
    builder->CreateCondBr(cond, forLoop->bodyBlock, forLoop->afterBlock);

    // set insertion point to body block for the loop contents
    builder->SetInsertPoint(forLoop->bodyBlock);

    return forLoop;
}

void completeForBody(void *block) {
    if (!block) {
        yyerror("Null block in completeForBody");
        return;
    }

    ForLoopBlock *forLoop = (ForLoopBlock *)block;

    // branch to increment block
    builder->CreateBr(forLoop->incBlock);

    // set up increment block
    builder->SetInsertPoint(forLoop->incBlock);
    Value *nextVal = builder->CreateFAdd(forLoop->iterVar, (Value *)createDoubleConstant(1.0), "i.next");

    // Use the saved pointer to update the original variable
    builder->CreateStore(nextVal, forLoop->originalVar);

    // branch back to condition
    builder->CreateBr(forLoop->condBlock);

    // continue with after block
    builder->SetInsertPoint(forLoop->afterBlock);

    delete forLoop;
}

// create a function
void *createFunction(const char *name) {
    if (!name || !builder || !context || !module) {
        yyerror("Invalid parameters or uninitialized LLVM");
        return nullptr;
    }

    FunctionBlock *funcBlock = new FunctionBlock();
    if (!funcBlock) {
        yyerror("Failed to allocate memory for function block");
        return nullptr;
    }

    // Create function type
    FunctionType *funcType = FunctionType::get(builder->getVoidTy(), false);

    // Create the function
    funcBlock->function = Function::Create(funcType, Function::ExternalLinkage, name, module);

    // Create entry block and return block
    funcBlock->entryBlock = BasicBlock::Create(*context, "entry", funcBlock->function);
    funcBlock->returnBlock = BasicBlock::Create(*context, "return", funcBlock->function);

    // Save current insertion point from main function
    BasicBlock *savedBlock = builder->GetInsertBlock();
    Function *savedFunction = savedBlock ? savedBlock->getParent() : nullptr;

    // Store the saved context so we can restore it later
    funcBlock->savedBlock = savedBlock;
    funcBlock->savedFunction = savedFunction;

    // Set insertion point to the functions entry block
    builder->SetInsertPoint(funcBlock->entryBlock);

    // Store function in table
    FunctionTable[name] = funcBlock->function;

    return funcBlock;
}

void completeFunction(void *block) {
    if (!block || !builder) {
        yyerror("Invalid parameters or uninitialized LLVM");
        return;
    }

    FunctionBlock *funcBlock = (FunctionBlock *)block;

    // create branch from entry block to return block if not already done
    // This is so if the function has no statements it still works
    if (!funcBlock->entryBlock->getTerminator()) {
        builder->CreateBr(funcBlock->returnBlock);
    }

    // Set insertion point to return block
    builder->SetInsertPoint(funcBlock->returnBlock);

    // Add return void instruction
    builder->CreateRetVoid();

    // Restore previous insertion point from main function
    if (funcBlock->savedBlock) {
        builder->SetInsertPoint(funcBlock->savedBlock);
    }

    delete funcBlock;
}

// Call a function
void callFunction(const char *name) {
    if (FunctionTable.find(name) != FunctionTable.end()) {
        // Create function call
        builder->CreateCall(FunctionTable[name], {});
    } else {
        yyerror("function not defined");
        exit(EXIT_FAILURE);
    }
}
