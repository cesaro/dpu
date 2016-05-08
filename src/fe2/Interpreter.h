//===-- Interpreter.h ------------------------------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header file defines the interpreter structure
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_EXECUTIONENGINE_INTERPRETER_INTERPRETER_H
#define LLVM_LIB_EXECUTIONENGINE_INTERPRETER_INTERPRETER_H

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
namespace llvm {

class IntrinsicLowering;
struct FunctionInfo;
template<typename T> class generic_gep_type_iterator;
class ConstantExpr;
typedef generic_gep_type_iterator<User::const_op_iterator> gep_type_iterator;


/// Printing generic values
raw_ostream &operator<<(raw_ostream &s, const std::pair<GenericValue*,Type*> &v);


// AllocaHolder - Object to track all of the blocks of memory allocated by
// alloca.  When the function returns, this object is popped off the execution
// stack, which causes the dtor to be run, which frees all the alloca'd memory.
//
class AllocaHolder {
  std::vector<void *> Allocations;

public:
  AllocaHolder() {}

  // Make this type move-only. Define explicit move special members for MSVC.
  AllocaHolder(AllocaHolder &&RHS) : Allocations(std::move(RHS.Allocations)) {}
  AllocaHolder &operator=(AllocaHolder &&RHS) {
    Allocations = std::move(RHS.Allocations);
    return *this;
  }

  ~AllocaHolder() {
    for (void *Allocation : Allocations)
      free(Allocation);
  }

  void add(void *Mem) { Allocations.push_back(Mem); }
};

typedef std::vector<GenericValue> ValuePlaneTy;

// ExecutionContext struct - This struct represents one stack frame currently
// executing.
//
struct ExecutionContext {
  Function             *CurFunction;// The currently executing function
  BasicBlock           *CurBB;      // The currently executing BB
  BasicBlock::iterator  CurInst;    // The next instruction to execute
  CallSite              Caller;     // Holds the call that called subframes.
                                    // NULL if main func or debugger invoked fn
  std::map<Value *, GenericValue> Values; // LLVM values used in this invocation
  std::vector<GenericValue>  VarArgs; // Values passed through an ellipsis
  AllocaHolder Allocas;            // Track memory allocated by alloca

  ExecutionContext() : CurFunction(nullptr), CurBB(nullptr), CurInst(nullptr) {}

  ExecutionContext(ExecutionContext &&O)
      : CurFunction(O.CurFunction), CurBB(O.CurBB), CurInst(O.CurInst),
        Caller(O.Caller), Values(std::move(O.Values)),
        VarArgs(std::move(O.VarArgs)), Allocas(std::move(O.Allocas)) {}

  ExecutionContext &operator=(ExecutionContext &&O) {
    CurFunction = O.CurFunction;
    CurBB = O.CurBB;
    CurInst = O.CurInst;
    Caller = O.Caller;
    Values = std::move(O.Values);
    VarArgs = std::move(O.VarArgs);
    Allocas = std::move(O.Allocas);
    return *this;
  }
};

struct GlobalState {
  std::vector<uint8_t> buff; // global variables, no heap

  size_t size() { return buff.size(); }
  void *offsetToProgramAddr (size_t offset) { return (char*) 16 + offset; }
  bool isProgramGlobalAddr (void *ptr) { return ptr < (char*) 16 + buff.size(); }
  void *translateGlobalProgramAddr (void *ptr) { return buff.data() + (size_t) ptr - 16; }
};

struct LocalState {
  std::vector<ExecutionContext> ecstack; // stack frames, top is current sf
  std::vector<uint8_t>          locals;  // memory obtained through allocas

  // mimic addGlobalMapping and getMemoryForGV
};

// Interpreter - This class represents the entirety of the interpreter.
//
class Interpreter : public ExecutionEngine, public InstVisitor<Interpreter> {
  GenericValue ExitValue;          // The return value of the called function
  DataLayout TD;
  IntrinsicLowering *IL;

  GlobalState *gs;
  LocalState  *ls;


  std::vector<ExecutionContext> ECStack; // stack frames, top is current sf

  // AtExitHandlers - List of functions to call when the program exits,
  // registered with the atexit() library function.
  std::vector<Function*> AtExitHandlers;

public:
  explicit Interpreter(std::unique_ptr<Module> M);
  ~Interpreter() override;

  /// runAtExitHandlers - Run any functions registered by the program's calls to
  /// atexit(3), which we intercept and store in AtExitHandlers.
  ///
  void runAtExitHandlers();

  static void Register() {
    InterpCtor = create;
  }

  /// Create an interpreter ExecutionEngine.
  ///
  static ExecutionEngine *create(std::unique_ptr<Module> M,
                                 std::string *ErrorStr = nullptr);

  /// run - Start execution with the specified function and arguments.
  ///
  GenericValue runFunction(Function *F,
                           ArrayRef<GenericValue> ArgValues) override;

  void *getPointerToNamedFunction(StringRef Name,
                                  bool AbortOnFailure = true) override {
    // FIXME: not implemented.
    return nullptr;
  }

  void addModule(std::unique_ptr<Module> M) override {
    report_fatal_error ("Interpreter: this class only accepts 1 module");
  }

  void StoreValueToMemory(const GenericValue &Val, GenericValue *Ptr, Type *Ty);
  void LoadValueFromMemory(GenericValue &Result, GenericValue *Ptr, Type *Ty);

  // Methods used to execute code:
  // Place a call on the stack
  void callFunction(Function *F, ArrayRef<GenericValue> ArgVals);
  void run();                // Execute instructions until nothing left to do

  // Opcode Implementations
  void visitReturnInst(ReturnInst &I);
  void visitBranchInst(BranchInst &I);
  void visitSwitchInst(SwitchInst &I);
  void visitIndirectBrInst(IndirectBrInst &I);

  void visitBinaryOperator(BinaryOperator &I);
  void visitICmpInst(ICmpInst &I);
  void visitFCmpInst(FCmpInst &I);
  void visitAllocaInst(AllocaInst &I);
  void visitLoadInst(LoadInst &I);
  void visitStoreInst(StoreInst &I);
  void visitGetElementPtrInst(GetElementPtrInst &I);
  void visitPHINode(PHINode &PN) { 
    llvm_unreachable("PHI nodes already handled!"); 
  }
  void visitTruncInst(TruncInst &I);
  void visitZExtInst(ZExtInst &I);
  void visitSExtInst(SExtInst &I);
  void visitFPTruncInst(FPTruncInst &I);
  void visitFPExtInst(FPExtInst &I);
  void visitUIToFPInst(UIToFPInst &I);
  void visitSIToFPInst(SIToFPInst &I);
  void visitFPToUIInst(FPToUIInst &I);
  void visitFPToSIInst(FPToSIInst &I);
  void visitPtrToIntInst(PtrToIntInst &I);
  void visitIntToPtrInst(IntToPtrInst &I);
  void visitBitCastInst(BitCastInst &I);
  void visitSelectInst(SelectInst &I);


  void visitCallSite(CallSite CS);
  void visitCallInst(CallInst &I) { visitCallSite (CallSite (&I)); }
  void visitInvokeInst(InvokeInst &I) { visitCallSite (CallSite (&I)); }
  void visitUnreachableInst(UnreachableInst &I);

  void visitShl(BinaryOperator &I);
  void visitLShr(BinaryOperator &I);
  void visitAShr(BinaryOperator &I);

  void visitVAArgInst(VAArgInst &I);
  void visitExtractElementInst(ExtractElementInst &I);
  void visitInsertElementInst(InsertElementInst &I);
  void visitShuffleVectorInst(ShuffleVectorInst &I);

  void visitExtractValueInst(ExtractValueInst &I);
  void visitInsertValueInst(InsertValueInst &I);

  void visitInstruction(Instruction &I) {
    errs() << I << "\n";
    llvm_unreachable("Instruction not interpretable yet!");
  }

  GenericValue callExternalFunction(Function *F,
                                    ArrayRef<GenericValue> ArgVals);
  void exitCalled(GenericValue GV);

  void addAtExitHandler(Function *F) {
    AtExitHandlers.push_back(F);
  }

  GenericValue *getFirstVarArg () {
    return &(ECStack.back ().VarArgs[0]);
  }

private:  // Helper functions
  GenericValue executeGEPOperation(Value *Ptr, gep_type_iterator I,
                                   gep_type_iterator E, ExecutionContext &SF);

  // SwitchToNewBasicBlock - Start execution in a new basic block and run any
  // PHI nodes in the top of the block.  This is used for intraprocedural
  // control flow.
  //
  void SwitchToNewBasicBlock(BasicBlock *Dest, ExecutionContext &SF);

  void *getPointerToFunction(Function *F) override { return (void*)F; }

  void initializeExecutionEngine() { }
  void initializeExternalFunctions();
  GenericValue getConstantExprValue(ConstantExpr *CE, ExecutionContext &SF);
  GenericValue getOperandValue(Value *V, ExecutionContext &SF);
  GenericValue executeTruncInst(Value *SrcVal, Type *DstTy,
                                ExecutionContext &SF);
  GenericValue executeSExtInst(Value *SrcVal, Type *DstTy,
                               ExecutionContext &SF);
  GenericValue executeZExtInst(Value *SrcVal, Type *DstTy,
                               ExecutionContext &SF);
  GenericValue executeFPTruncInst(Value *SrcVal, Type *DstTy,
                                  ExecutionContext &SF);
  GenericValue executeFPExtInst(Value *SrcVal, Type *DstTy,
                                ExecutionContext &SF);
  GenericValue executeFPToUIInst(Value *SrcVal, Type *DstTy,
                                 ExecutionContext &SF);
  GenericValue executeFPToSIInst(Value *SrcVal, Type *DstTy,
                                 ExecutionContext &SF);
  GenericValue executeUIToFPInst(Value *SrcVal, Type *DstTy,
                                 ExecutionContext &SF);
  GenericValue executeSIToFPInst(Value *SrcVal, Type *DstTy,
                                 ExecutionContext &SF);
  GenericValue executePtrToIntInst(Value *SrcVal, Type *DstTy,
                                   ExecutionContext &SF);
  GenericValue executeIntToPtrInst(Value *SrcVal, Type *DstTy,
                                   ExecutionContext &SF);
  GenericValue executeBitCastInst(Value *SrcVal, Type *DstTy,
                                  ExecutionContext &SF);
#if 0
  GenericValue executeCastOperation(Instruction::CastOps opcode, Value *SrcVal, 
                                    Type *Ty, ExecutionContext &SF);
#endif
  void popStackAndReturnValueToCaller(Type *RetTy, GenericValue Result);


  void relocateGlobalVariables ();
  void *translateProgramAddr (void *ptr);
};

} // End llvm namespace

#endif

// vim:ts=2 sw=2 et:
