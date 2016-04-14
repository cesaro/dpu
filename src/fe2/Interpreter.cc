//===- Interpreter.cpp - Top-Level LLVM Interpreter Implementation --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the top-level functionality for the LLVM interpreter.
// This interpreter is designed to be a very simple, portable, inefficient
// interpreter.
//
//===----------------------------------------------------------------------===//

#include "Interpreter.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Format.h"
#include <cstring>
using namespace llvm;

namespace {

static struct RegisterInterp {
  RegisterInterp() { Interpreter::Register(); }
} InterpRegistrator;

}

extern "C" void LLVMLinkInInterpreter() { }

/// Create a new interpreter object.
///
ExecutionEngine *Interpreter::create(std::unique_ptr<Module> M,
                                     std::string *ErrStr) {
  // Tell this Module to materialize everything and release the GVMaterializer.
  if (std::error_code EC = M->materializeAllPermanently()) {
    if (ErrStr)
      *ErrStr = EC.message();
    // We got an error, just return 0
    return nullptr;
  }

  return new Interpreter(std::move(M));
}

//===----------------------------------------------------------------------===//
// Interpreter ctor - Initialize stuff
//
Interpreter::Interpreter(std::unique_ptr<Module> M)
  : ExecutionEngine(std::move(M)), TD(Modules.back().get()) {

  memset(&ExitValue.Untyped, 0, sizeof(ExitValue.Untyped));
  setDataLayout(&TD);
  // Initialize the "backend"
  initializeExecutionEngine();
  initializeExternalFunctions();
  emitGlobals();

  IL = new IntrinsicLowering(TD);
  gs = new GlobalState ();
  ls = new LocalState ();

  relocateGlobalVariables ();
}

Interpreter::~Interpreter() {
  delete IL;
}

void Interpreter::runAtExitHandlers () {
  while (!AtExitHandlers.empty()) {
    callFunction(AtExitHandlers.back(), None);
    AtExitHandlers.pop_back();
    run();
  }
}

/// run - Start execution with the specified function and arguments.
///
GenericValue Interpreter::runFunction(Function *F,
                                      ArrayRef<GenericValue> ArgValues) {
  assert (F && "Function *F was null at entry to run()");

  // Try extra hard not to pass extra args to a function that isn't
  // expecting them.  C programmers frequently bend the rules and
  // declare main() with fewer parameters than it actually gets
  // passed, and the interpreter barfs if you pass a function more
  // parameters than it is declared to take. This does not attempt to
  // take into account gratuitous differences in declared types,
  // though.
  const size_t ArgCount = F->getFunctionType()->getNumParams();
  ArrayRef<GenericValue> ActualArgs =
      ArgValues.slice(0, std::min(ArgValues.size(), ArgCount));

  // Set up the function call.
  callFunction(F, ActualArgs);

  // Start executing the function.
  run();

  return ExitValue;
}

void Interpreter::relocateGlobalVariables() {

  size_t offset = 0;
  std::vector<std::pair<GlobalVariable*,size_t>> tab;

  // if there is more than module, we would have to deal linking aspects
  if (Modules.size () != 1)
    report_fatal_error ("relocateGlobalVariables: not multi module support");

  // associate an offset in memory for all global variables
  for (auto & m : Modules) {
    for (auto & g : m->globals ()) {
      // retrieve the size and alignment of the global variable
      unsigned align = TD.getPreferredAlignment (&g);
      size_t   size  = TD.getTypeAllocSize (g.getType()->getElementType());

      // compute the offset in memory for this global and store it in tab
      offset = RoundUpToAlignment (offset, align);
      tab.push_back (std::make_pair (&g, offset));

      // print
      outs() << "m " << m.get() << " g " << &g;
      outs() << " offset " << offset;
      outs() << " size " << size;
      outs() << " align " << align;
      outs() << " curraddr " << getPointerToGlobalIfAvailable (&g);
      outs() << " progaddr " << gs->offsetToProgramAddr (offset);
      outs() << " name '" << g.getName() << "'";
      outs() << " ty ";
      g.getType()->print (outs());
      outs() << "\n";

      offset += size;
    }
  }

  // undo the work done in emitGlobals()
  clearAllGlobalMappings ();

  // allocate memory in the gs object, store initial values, set up global
  // mapping
  gs->buff.clear();
  gs->buff.resize (offset);
  gs->buff.shrink_to_fit();
  for (auto & p : tab) {
    InitializeMemory (p.first->getInitializer(), gs->buff.data() + p.second);
    addGlobalMapping (p.first, gs->offsetToProgramAddr (p.second));
  }
}

void *Interpreter::translateProgramAddr (void *ptr) {

  void *p;

  if (gs->isProgramGlobalAddr (ptr))
    p = gs->translateGlobalProgramAddr (ptr);
  else
    p = ptr;
  outs() << format ("translateGlobalProgramAddr: %p -> %p\n", ptr, p);
  return p;
}

/// LoadIntFromMemory - Loads the integer stored in the LoadBytes bytes starting
/// from Src into IntVal, which is assumed to be wide enough and to hold zero.
/// This is taken as is from ExecutionEngine.cpp
static void LoadIntFromMemory(APInt &IntVal, uint8_t *Src, unsigned LoadBytes) {
  assert((IntVal.getBitWidth()+7)/8 >= LoadBytes && "Integer too small!");
  uint8_t *Dst = reinterpret_cast<uint8_t *>(
                   const_cast<uint64_t *>(IntVal.getRawData()));

  if (sys::IsLittleEndianHost)
    // Little-endian host - the destination must be ordered from LSB to MSB.
    // The source is ordered from LSB to MSB: Do a straight copy.
    memcpy(Dst, Src, LoadBytes);
  else {
    // Big-endian - the destination is an array of 64 bit words ordered from
    // LSW to MSW.  Each word must be ordered from MSB to LSB.  The source is
    // ordered from MSB to LSB: Reverse the word order, but not the bytes in
    // a word.
    while (LoadBytes > sizeof(uint64_t)) {
      LoadBytes -= sizeof(uint64_t);
      // May not be aligned so use memcpy.
      memcpy(Dst, Src + LoadBytes, sizeof(uint64_t));
      Dst += sizeof(uint64_t);
    }

    memcpy(Dst + sizeof(uint64_t) - LoadBytes, Src, LoadBytes);
  }
}

/// StoreIntToMemory - Fills the StoreBytes bytes of memory starting from Dst
/// with the integer held in IntVal.
static void StoreIntToMemory(const APInt &IntVal, uint8_t *Dst,
                             unsigned StoreBytes) {
  assert((IntVal.getBitWidth()+7)/8 >= StoreBytes && "Integer too small!");
  const uint8_t *Src = (const uint8_t *)IntVal.getRawData();

  if (sys::IsLittleEndianHost) {
    // Little-endian host - the source is ordered from LSB to MSB.  Order the
    // destination from LSB to MSB: Do a straight copy.
    memcpy(Dst, Src, StoreBytes);
  } else {
    // Big-endian host - the source is an array of 64 bit words ordered from
    // LSW to MSW.  Each word is ordered from MSB to LSB.  Order the destination
    // from MSB to LSB: Reverse the word order, but not the bytes in a word.
    while (StoreBytes > sizeof(uint64_t)) {
      StoreBytes -= sizeof(uint64_t);
      // May not be aligned so use memcpy.
      memcpy(Dst + StoreBytes, Src, sizeof(uint64_t));
      Src += sizeof(uint64_t);
    }

    memcpy(Dst, Src + sizeof(uint64_t) - StoreBytes, StoreBytes);
  }
}


// overriding the same function in ExecutionEngine, as the Ptr is now to be
// interpreted
void Interpreter::StoreValueToMemory(const GenericValue &Val,
                                         GenericValue *Ptr, Type *Ty) {
  const unsigned StoreBytes = getDataLayout()->getTypeStoreSize(Ty);

  // Ptr is a pointer in the address space of the interpreted program, which
  // maps to a real pointer either inside of gs->buff or ls->locals
  void * p = translateProgramAddr (Ptr);

  outs() << "StoreValueToMemory: Ptr " << Ptr;
  outs() << " isGlobalAddr " << gs->isProgramGlobalAddr (Ptr);
  outs() << " p " << p;
  outs() << " glboff " << (size_t) p - (size_t) gs->buff.data();
  outs() << " size " << StoreBytes;
  outs() << "\n";

  switch (Ty->getTypeID()) {
  default:
    {
      SmallString<256> Msg;
      raw_svector_ostream OS(Msg);
      OS << "Cannot store value of type " << *Ty << "!";
      report_fatal_error(OS.str());
      break;
    }
  case Type::IntegerTyID:
    StoreIntToMemory(Val.IntVal, (uint8_t*)p, StoreBytes);
    break;
  case Type::FloatTyID:
    *((float*)p) = Val.FloatVal;
    break;
  case Type::DoubleTyID:
    *((double*)p) = Val.DoubleVal;
    break;
  case Type::X86_FP80TyID:
    memcpy(p, Val.IntVal.getRawData(), 10);
    break;
  case Type::PointerTyID:
    // Ensure 64 bit target pointers are fully initialized on 32 bit hosts.
    if (StoreBytes != sizeof(PointerTy))
      memset(p, 0, StoreBytes);

    *((PointerTy*)p) = Val.PointerVal;
    break;
  case Type::VectorTyID:
    for (unsigned i = 0; i < Val.AggregateVal.size(); ++i) {
      if (cast<VectorType>(Ty)->getElementType()->isDoubleTy())
        *(((double*)p)+i) = Val.AggregateVal[i].DoubleVal;
      if (cast<VectorType>(Ty)->getElementType()->isFloatTy())
        *(((float*)p)+i) = Val.AggregateVal[i].FloatVal;
      if (cast<VectorType>(Ty)->getElementType()->isIntegerTy()) {
        unsigned numOfBytes =(Val.AggregateVal[i].IntVal.getBitWidth()+7)/8;
        StoreIntToMemory(Val.AggregateVal[i].IntVal, 
          (uint8_t*)p + numOfBytes*i, numOfBytes);
      }
    }
    break;
  }

  if (sys::IsLittleEndianHost != getDataLayout()->isLittleEndian())
    // Host and target are different endian - reverse the stored bytes.
    std::reverse((uint8_t*)p, StoreBytes + (uint8_t*)p);
}

// overriding the same function in ExecutionEngine, as the Ptr is now to be
// interpreted
void Interpreter::LoadValueFromMemory(GenericValue &Result,
                                          GenericValue *Ptr,
                                          Type *Ty) {
  const unsigned LoadBytes = getDataLayout()->getTypeStoreSize(Ty);

  // Ptr is a pointer in the address space of the interpreted program, which
  // maps to a real pointer either inside of gs->buff or ls->locals
  void * p = translateProgramAddr (Ptr);

  outs() << "LoadValueFromMemory: Ptr " << Ptr;
  outs() << " isGlobalAddr " << gs->isProgramGlobalAddr (Ptr);
  outs() << " p " << p;
  outs() << " glboff " << (size_t) p - (size_t) gs->buff.data();
  outs() << " size " << LoadBytes;
  outs() << "\n";

  switch (Ty->getTypeID()) {
  case Type::IntegerTyID:
    // An APInt with all words initially zero.
    Result.IntVal = APInt(cast<IntegerType>(Ty)->getBitWidth(), 0);
    LoadIntFromMemory(Result.IntVal, (uint8_t*)p, LoadBytes);
    break;
  case Type::FloatTyID:
    Result.FloatVal = *((float*)p);
    break;
  case Type::DoubleTyID:
    Result.DoubleVal = *((double*)p);
    break;
  case Type::PointerTyID:
    Result.PointerVal = *((PointerTy*)p);
    break;
  case Type::X86_FP80TyID: {
    // This is endian dependent, but it will only work on x86 anyway.
    // FIXME: Will not trap if loading a signaling NaN.
    uint64_t y[2];
    memcpy(y, p, 10);
    Result.IntVal = APInt(80, y);
    break;
  }
  case Type::VectorTyID: {
    const VectorType *VT = cast<VectorType>(Ty);
    const Type *ElemT = VT->getElementType();
    const unsigned numElems = VT->getNumElements();
    if (ElemT->isFloatTy()) {
      Result.AggregateVal.resize(numElems);
      for (unsigned i = 0; i < numElems; ++i)
        Result.AggregateVal[i].FloatVal = *((float*)p+i);
    }
    if (ElemT->isDoubleTy()) {
      Result.AggregateVal.resize(numElems);
      for (unsigned i = 0; i < numElems; ++i)
        Result.AggregateVal[i].DoubleVal = *((double*)p+i);
    }
    if (ElemT->isIntegerTy()) {
      GenericValue intZero;
      const unsigned elemBitWidth = cast<IntegerType>(ElemT)->getBitWidth();
      intZero.IntVal = APInt(elemBitWidth, 0);
      Result.AggregateVal.resize(numElems, intZero);
      for (unsigned i = 0; i < numElems; ++i)
        LoadIntFromMemory(Result.AggregateVal[i].IntVal,
          (uint8_t*)p+((elemBitWidth+7)/8)*i, (elemBitWidth+7)/8);
    }
    break;
  }
  default:
    SmallString<256> Msg;
    raw_svector_ostream OS(Msg);
    OS << "Cannot load value of type " << *Ty << "!";
    report_fatal_error(OS.str());
  }
}

raw_ostream &llvm::operator<<(raw_ostream &s, const std::pair<GenericValue*,Type*> &v)
{
  Type * t = v.second;
  GenericValue * vv = v.first;;

  switch (t->getTypeID()) {
  case Type::IntegerTyID:
  case Type::X86_FP80TyID:
    vv->IntVal.print (s, true);
    break;
  case Type::FloatTyID:
    s << vv->FloatVal;
    break;
  case Type::DoubleTyID:
    s << vv->DoubleVal;
    break;
  case Type::PointerTyID:
    s << vv->PointerVal;
    break;
  case Type::VectorTyID:
  default:
    s << "(type";
    s << *t << ")";
  }
  return s;
}


// vim:ts=2 sw=2 et:
