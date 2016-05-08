
#include <vector>
#include <utility>
#include <stdlib.h>

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/MCJIT.h"

#undef DEBUG // exported by ExecutionEngine.h
#include "verbosity.h"
#include "misc.hh"
#include "rt/rt.h"
#include "fe3/executor.hh"

using namespace dpu;

namespace dpu {
class Instrumenter : public llvm::InstVisitor<Instrumenter>
{
   llvm::LLVMContext *ctx;
   llvm::Module   *m;
   llvm::Function *ld8;
   llvm::Function *ld16;
   llvm::Function *ld32;
   llvm::Function *ld64;
   llvm::Function *st8;
   llvm::Function *st16;
   llvm::Function *st32;
   llvm::Function *st64;
   llvm::Function *allo;
   llvm::Function *mllo;
   llvm::Function *rllo;
   llvm::Function *free;
   llvm::Function *call;
   llvm::Function *ret;
   size_t count;

public:

   bool find_rt ()
   {
      ld8  = m->getFunction ("dpu_rt_load8");
      ld16 = m->getFunction ("dpu_rt_load16");
      ld32 = m->getFunction ("dpu_rt_load32");
      ld64 = m->getFunction ("dpu_rt_load64");

      st8  = m->getFunction ("dpu_rt_store8");
      st16 = m->getFunction ("dpu_rt_store16");
      st32 = m->getFunction ("dpu_rt_store32");
      st64 = m->getFunction ("dpu_rt_store64");

      allo = m->getFunction ("dpu_rt_allo");
      mllo = m->getFunction ("dpu_rt_mllo");
      rllo = m->getFunction ("dpu_rt_rllo");
      free = m->getFunction ("dpu_rt_fre");
      call = m->getFunction ("dpu_rt_call");
      ret  = m->getFunction ("dpu_rt_ret");

      return ld32 != nullptr and st32 != nullptr; // for instance
   }

   bool is_rt_fun (llvm::Function *f)
   {
      return f->getName().startswith ("dpu_rt_");
   }

   bool instrument (llvm::Module &m)
   {
      this->m = &m;
      ctx = &m.getContext ();
      if (not find_rt ()) return false;

      // improve this, the Instrument class will receive the list of functions
      // that it needs to instrument

      for (auto &f : m)
      {
         if (is_rt_fun (&f) or f.isDeclaration ()) continue;

         llvm::outs() << "dpu: fe3: instrumenting function '" << f.getName() << "'\n";
         count = 0;
         visit (f);
         llvm::outs() << "dpu: fe3: done, " << count << " instructions instrumented.\n";

         for (auto i : f.users())
         {
            llvm::outs() << "dpu: fe3:  used: " << *i << "\n";
         }
      }

      // check that we didn't do anything stupid
      llvm::verifyModule (m, &llvm::outs());

      return true;
   }

   void visitLoadInst (llvm::LoadInst &i)
   {
      llvm::IRBuilder<> b (i.getNextNode ());
      llvm::Value *addr;
      llvm::Value *v;
      llvm::Type *t;
      llvm::Function *f;

      //static int count = 0;
      //count++;
      //if (count >= 3) return;
      //llvm::outs() << "dpu: fe3: " << i << "\n";

      // if we are tring to load a pointer, make a bitcast to uint64_t
      addr = i.getPointerOperand ();
      v = &i;
      if (i.getType()->isPointerTy())
      {
         // t = i64*, address is bitcasted to type t
         t = llvm::Type::getInt64PtrTy (*ctx, addr->getType()->getPointerAddressSpace());
         addr = b.CreateBitCast (addr, t, "imnt");
         // lodaded value is converted to i64
         v = b.CreatePtrToInt (&i, b.getInt64Ty(), "imnt");
      }

      // check if we support the bitwith
      // use isSized() + queries to the DataLayaout system to generalize this
      if (not v->getType()->isIntegerTy())
         throw std::runtime_error ("Instrumentation: load: non-integer type");
      switch (v->getType()->getIntegerBitWidth ())
      {
      case 8 : f = ld8; break;
      case 16 : f = ld16; break;
      case 32 : f = ld32; break;
      case 64 : f = ld64; break;
      default :
         throw std::runtime_error ("Instrumentation: load: cannot handle bitwith");
      }

      // instruction to call to the runtime
      b.CreateCall (f, {addr, v});
      count++;
   }

   void visitStoreInst (llvm::StoreInst &i)
   {
      llvm::IRBuilder<> b (i.getNextNode ());
      llvm::Value *addr;
      llvm::Value *v;
      llvm::Type *t;
      llvm::Function *f;

      //static int count = 0;
      //count++;
      //if (count >= 3) return;
      //llvm::outs() << "dpu: fe3: " << i << "\n";

      // if we are tring to store a pointer, make a bitcast to uint64_t
      v = i.getValueOperand ();
      addr = i.getPointerOperand ();
      if (v->getType()->isPointerTy())
      {
         // t = i64*, address is bitcasted to type t
         t = llvm::Type::getInt64PtrTy (*ctx, addr->getType()->getPointerAddressSpace());
         addr = b.CreateBitCast (addr, t, "imnt");
         // lodaded value is converted to i64
         v = b.CreatePtrToInt (v, b.getInt64Ty(), "imnt");
      }

      // check if we support the bitwith
      if (not v->getType()->isIntegerTy())
         throw std::runtime_error ("Instrumentation: store: non-integer type");
      switch (v->getType()->getIntegerBitWidth ())
      {
      case 8 : f = st8; break;
      case 16 : f = st16; break;
      case 32 : f = st32; break;
      case 64 : f = st64; break;
      default :
         throw std::runtime_error ("Instrumentation: store: cannot handle bitwith");
      }

      // instruction to call to the runtime
      b.CreateCall (f, {addr, v});
      count++;
   }

   void visitAllocaInst (llvm::AllocaInst &i)
   {
      llvm::IRBuilder<> b (i.getNextNode ());
      llvm::Value *addr;
      llvm::Value *size;
      uint32_t ts;

      //static int count = 0;
      //count++;
      //if (count >= 3) return;
      //llvm::outs() << "dpu: fe3: " << i << "\n";

      // get the target size of the allocated type (truncate to 32 bits)
      ts = m->getDataLayout().getTypeStoreSize (i.getAllocatedType());
      // s = m->getDataLayout().getTypeAllocSize (i.getAllocatedType()); // exclude pad space
      
      // multiply type size times number of elements in the array, in 32bits
      size = b.CreateMul (b.getInt32 (ts),
            b.CreateZExtOrTrunc (i.getArraySize(), b.getInt32Ty(), "imnt"), "imnt");
      addr = b.CreateBitCast (&i, b.getInt8PtrTy(), "imnt"); // address space ?
      b.CreateCall (allo, {addr, size});
      count++;
   }

   void visitReturnInst (llvm::ReturnInst &i)
   {
      static uint32_t retid = 0;
      llvm::IRBuilder<> b (&i);

      llvm::outs() << "dpu: fe3: " << i << "\n";

      b.CreateCall (ret, b.getInt32 (retid++));
      count++;
   }

   // visit(M) in parent class should never be called ;)
   void visitModule (llvm::Module &m) { ASSERT (0); }
};
} // namespace dpu


Executor::Executor (std::unique_ptr<llvm::Module> mod, ExecutorConfig c) :
      initialized (false),
      conf (c),
      ctx (mod->getContext ()),
      m (mod.get ()),
      ee (0)
{
   std::string errors;
   llvm::EngineBuilder eb (std::move(mod));

   // make sure we can safely destroy the object
   rt.memstart = 0;
   rt.trace.evstart = 0;
   rt.trace.addrstart = 0;
   rt.trace.idstart = 0;
   rt.trace.valstart = 0;

   // create a JIT execution engine
   eb.setErrorStr (&errors);
   eb.setOptLevel (llvm::CodeGenOpt::Level::Aggressive);
   eb.setMCJITMemoryManager(llvm::make_unique<llvm::SectionMemoryManager>());
   ee = eb.create();
   if (! ee)
   {
      std::string s = fmt ("Executor: unable to create ExecutionEngine: %s", errors.c_str());
      throw std::runtime_error (s);
   }

   // tell the module the way the target lays out data structures
   m->setDataLayout (*ee->getDataLayout());
}

Executor::~Executor ()
{
   free ((void*) rt.memstart);
   free ((void*) rt.trace.evstart);
   free ((void*) rt.trace.addrstart);
   free ((void*) rt.trace.idstart);
   free ((void*) rt.trace.valstart);
   delete ee;
}

void Executor::initialize ()
{
   if (initialized) return;
   initialize_rt ();
   instrument_module ();
   initialized = true;
}

void Executor::initialize_rt ()
{
   // allocate the memory space for the guest code (heap + stacks)
   rt.memsize = conf.memsize;
   rt.memstart = (uint64_t) malloc (conf.memsize);
   if (rt.memstart == 0)
      throw std::runtime_error ("malloc: cannot prepare memory for code execution");
   rt.memend = rt.memstart + rt.memsize;

   // the stacks should fit into the main memory
   ASSERT (conf.stacksize < conf.memsize);
   // stacks are located at the end of the memory
   rt.stacksize = conf.stacksize;
   rt.stackend = rt.memend;
   rt.stackstart = rt.stackend - rt.stacksize;

   // allocate memory for the event trace
   ASSERT (EVFULL < 256);
   rt.trace.evstart = (uint8_t *) malloc (conf.tracesize);
   if (rt.trace.evstart == 0)
      throw std::runtime_error ("malloc: cannot allocate memory for log trace");
   rt.trace.evend = rt.trace.evstart + conf.tracesize;
   rt.trace.evptr = rt.trace.evstart;

   // addr operands
   rt.trace.addrstart = (uint64_t *) malloc (conf.tracesize * sizeof (uint64_t));
   if (rt.trace.addrstart == 0)
      throw std::runtime_error ("malloc: cannot allocate memory for log trace");
   rt.trace.addrptr = rt.trace.addrstart;

   // id operands
   rt.trace.idstart = (uint16_t *) malloc (conf.tracesize * sizeof (uint16_t));
   if (rt.trace.idstart == 0)
      throw std::runtime_error ("malloc: cannot allocate memory for log trace");
   rt.trace.idptr = rt.trace.idstart;

   // val operands
   rt.trace.valstart = (uint64_t *) malloc (conf.tracesize * sizeof (uint64_t));
   if (rt.trace.valstart == 0)
      throw std::runtime_error ("malloc: cannot allocate memory for log trace");
   rt.trace.valptr = rt.trace.valstart;

   rt.host_rsp = 0;

   // instrument the module to use this->rt as state
   llvm::GlobalVariable *g;
   llvm::Type *t;
   g = m->getGlobalVariable ("rt", true);
   t = m->getTypeByName ("struct.rt");
   if (!g or !t) throw std::runtime_error ("Executor: input missing runtime");
   g->setInitializer (ptr_to_llvm (&rt, t));
   llvm::outs() << "instrumeted rt pointers:\n";
   llvm::outs() << "- " << *g << "\n";

   // similarly for the other const global variables
   std::vector<std::pair<const char*, uint64_t>> pairs =
      { std::make_pair ("memstart", rt.memstart),
        std::make_pair ("memend", rt.memend),
        std::make_pair ("evend", (uint64_t) rt.trace.evend) };
   for (auto &p : pairs)
   {
      g = m->getGlobalVariable (p.first, true);
      if (!g) throw std::runtime_error ("Executor: input missing runtime");
      g->setInitializer (llvm::ConstantInt::get
            (llvm::Type::getInt64Ty (ctx), p.second));
      llvm::outs() << "- " << *g << "\n";
   }
}

void Executor::instrument_module ()
{
   // instrument the code
   Instrumenter i;
   if (not i.instrument (*m))
   {
      throw std::runtime_error ("Executor: rt missing in input module");
   }
}

void Executor::run ()
{
   void *ptr;
   int (*entry) (int, const char* const*, const char* const*);

   // make sure that argv and envp members have the right null pointer at the
   // end
   ASSERT (argv.size() >= 1 and argv[0] != 0);
   ASSERT (envp.size() and envp.back() == 0);

   // ask LLVM to JIT the program
   ee->finalizeObject ();
   ptr = (void *) ee->getFunctionAddress ("dpu_rt_start");
   entry = (int (*) (int, const char* const*, const char* const*)) ptr;

   // run the user program!!
   exitcode = entry (argv.size(), argv.data(), envp.data());
}

llvm::Constant *Executor::ptr_to_llvm (void *ptr, llvm::Type *t)
{
   llvm::Constant *c;

   // generate integer
   c = llvm::ConstantInt::get (llvm::Type::getInt64Ty (ctx), (uint64_t) ptr);
   // convert it into pointer to type t
   c = llvm::ConstantExpr::getIntToPtr (c, t->getPointerTo (0));
   return c;
}
