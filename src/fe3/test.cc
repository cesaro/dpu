
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstdio>
#include <vector>

#include "llvm/Pass.h"
#include "llvm/PassSupport.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/MCJIT.h"

#undef DEBUG // exported by ExecutionEngine.h
#include "verbosity.h"

#include "fe3/test.hh"
#include "fe3/executor.hh"
#include "rt/rt.h"

using namespace dpu::fe3;
using namespace dpu;

void ir_write_ll (const llvm::Module *m, const char *filename)
{
   int fd = open (filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *m;
}

void ir_write_bc (const llvm::Module *m, const char *filename)
{
   int fd = open (filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   llvm::WriteBitcodeToFile (m, f);
}

class HelloFunctionPass : public llvm::FunctionPass
{
public:
   static char ID;
   HelloFunctionPass () : llvm::FunctionPass (ID) {}

   bool runOnFunction (llvm::Function & f)
   {
      DEBUG ("pass %p f %p f.name '%s'", this, &f, f.getName ());
#if 0
      for (auto &b : f)
      {
         DEBUG ("  b %p name '%s' size %zd", &b, b.getName (), b.size ());
         for (auto &ins : b)
         {
            DEBUG ("     i %p", &ins);
            fflush (stdout);
            ins.dump ();
            llvm::errs().flush();
         }
      }
#else
      for (llvm::inst_iterator i = llvm::inst_begin (f), e = llvm::inst_end (f); i != e; ++i)
      {
         DEBUG ("  i %p", &*i);
         fflush (stdout);
         i->dump ();
         llvm::errs().flush();
      }
#endif
      return false;
   }
};

char HelloFunctionPass::ID = 0;
static llvm::RegisterPass<HelloFunctionPass> tmp
      ("cesar", "Hello function pass", false, false);

class Instrument : public llvm::InstVisitor<Instrument>
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
   llvm::Function *ret;
   llvm::Function *start;
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

      allo = m->getFunction ("dpu_rt_alloca");
      mllo = m->getFunction ("dpu_rt_malloc");
      ret  = m->getFunction ("dpu_rt_ret");
      start= m->getFunction ("dpu_rt_start");

      return ld8 != nullptr and st8 != nullptr;
   }

   llvm::Function *get_entry ()
   {
      return start;
   }

   bool is_rt_fun (llvm::Function *f)
   {
      return f->getName().startswith ("dpu_rt_");
   }

   bool instrument (llvm::Module &m)
   {
      this->m =  &m;
      ctx = &m.getContext ();
      if (not find_rt ()) return false;

      // improve this, the Instrument class will receive the list of functions
      // that it needs to instrument

      for (auto &f : m)
      {
         if (is_rt_fun (&f) or f.isDeclaration ()) continue;
         //if (f.getName().equals ("main")) continue; // hack for the experiments, update

         llvm::outs() << "dpu: fe3: instrumenting function '" << f.getName() << "'\n";
         count = 0;
         visit (f);
         llvm::outs() << "dpu: fe3: done, " << count << " instructions.\n";

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

void fe3_test1 ()
{
   // get a context
   llvm::LLVMContext &context = llvm::getGlobalContext();
   llvm::SMDiagnostic err;

   // file to load and execute
   std::string path = "input.ll";

   // parse the .ll file and get a Module out of it
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (path, err, context));
   llvm::Module * m = mod.get();

   // if errors found, write them to errors and return
   if (! mod.get ()) {
      std::string errors;
      llvm::raw_string_ostream os (errors);
      err.print (path.c_str(), os);
      os.flush ();
      printf ("Error: %s\n", errors.c_str());
      return;
   }

   printf ("functions in the module:\n");
   for (auto & f : m->functions())
      DEBUG ("- m %p fun %p decl %d name %s", m, &f, f.isDeclaration(), f.getName().str().c_str());
   fflush (stdout);
   llvm::outs().flush();
   llvm::errs().flush();

   // create a function pass manager
   printf ("creating fpm:\n");
   fflush (stdout);
   llvm::legacy::FunctionPassManager fpm (m);

   // register a new pass
   fpm.add (new HelloFunctionPass ());

   // run our pass on all functions
   printf ("running the pass manager:\n");
   fflush (stdout);
   for (auto & f : m->functions())
   {
      //DEBUG ("- optimizing fun %p name %s", &f, f.getName ());
      //fflush (stdout);
      fpm.run (f);
   }
   fflush (stdout);
}
int callit (void *ptr)
{
   int (*entry) (int,char**) = (int (*) (int, char**)) ptr;
   char argv0[] = "a";
   char argv1[] = "b";
   char argv2[] = "c";
   char *argv[3] = { argv0, argv1, argv2 };

   DEBUG ("first time xxxxxxxxxxxxxxxxxxx");
   return entry (3, argv);
}

void fe3_test2 ()
{
   // related to the JIT engine
   llvm::InitializeNativeTarget();
   llvm::InitializeNativeTargetAsmPrinter();
   llvm::InitializeNativeTargetAsmParser();

   // get a context
   llvm::LLVMContext &context = llvm::getGlobalContext();
   llvm::SMDiagnostic err;
   std::string errors;

   // file to load and execute
   std::string path = "input.ll";
   //std::string path = "cunf.ll";
   //std::string path = "invalid.ll";

   // parse the .ll file and get a Module out of it
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (path, err, context));
   llvm::Module * m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (path.c_str(), os);
      os.flush ();
      printf ("Error: %s\n", errors.c_str());
      return;
   }

   printf ("functions in the module:\n");
   for (auto & f : *m)
      DEBUG ("- m %p fun %p decl %d name %s", m, &f, f.isDeclaration(), f.getName().str().c_str());
   fflush (stdout);

   // create a JIT execution engine
   llvm::EngineBuilder eb (std::move(mod));
   eb.setErrorStr (&errors);
   //eb.setOptLevel ( !! );
   eb.setMCJITMemoryManager(llvm::make_unique<llvm::SectionMemoryManager>());

   llvm::ExecutionEngine *ee = eb.create();
   if (! ee)
   {
      printf ("Error: unable to create ExecutionEngine: %s\n", errors.c_str());
      return;
   }

   // tell the module the way the target lays out data structures
   m->setDataLayout (*ee->getDataLayout());

   // instrument the code
   Instrument instrm;
   if (not instrm.instrument (*m))
   {
      DEBUG ("rt not found in the module, aborting");
      return;
   }

   // write code to file
   ir_write_ll (m, "out.ll");
   DEBUG ("dpu: module saved to 'out.ll'");
   return;

   // execute it
   ee->finalizeObject ();
   void * ptr = (void *) ee->getFunctionAddress ("dpu_rt_start");
   int (*entry) (int,char**,char**) = (int (*) (int, char**, char**)) ptr;
   char argv0[] = "a";
   char argv1[] = "b";
   char argv2[] = "c";
   char *argv[3] = { argv0, argv1, argv2 };
   //char *env[1] = { 0 };

   DEBUG ("first time xxxxxxxxxxxxxxxxxxx");
   int ret1 = entry (3, argv, argv);
   //DEBUG ("second time xxxxxxxxxxxxxxxxxxx");
   //int ret2 = entry (3, argv);
   //int ret2 = callit (ptr);
   DEBUG ("done xxxxxxxxxxxxxxxxxxx");

   //DEBUG ("dpu: entry returned %d %d", ret1, ret2);
   DEBUG ("dpu: entry returned %d", ret1);
}

void fe3_test3 ()
{
   // related to the JIT engine
   llvm::InitializeNativeTarget();
   llvm::InitializeNativeTargetAsmPrinter();
   llvm::InitializeNativeTargetAsmParser();

   // get a context
   llvm::LLVMContext &context = llvm::getGlobalContext();
   llvm::SMDiagnostic err;
   std::string errors;

   // file to load and execute
   std::string path = "input.ll";
   //std::string path = "cunf.ll";
   //std::string path = "invalid.ll";

   // parse the .ll file and get a Module out of it
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (path, err, context));
   llvm::Module * m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (path.c_str(), os);
      os.flush ();
      printf ("Error: %s\n", errors.c_str());
      return;
   }

   printf ("functions in the module:\n");
   for (auto & f : *m)
      DEBUG ("- m %p fun %p decl %d name %s", m, &f, f.isDeclaration(), f.getName().str().c_str());
   fflush (stdout);
   printf ("globals in the module:\n");
   for (auto & g : m->globals()) llvm::errs() << "- g " << &g << " dump " << g << "\n";
   fflush (stdout);

   // prepare an executor
   ExecutorConfig conf;
   conf.memsize = 1 << 30; // 1G
   conf.stacksize = 16 << 20; // 16M
   conf.tracesize = 8 << 20; // 8M events (x 11 bytes per event)
   Executor e (std::move (mod), conf);
   e.initialize ();

   // write code to file
   ir_write_ll (m, "out.ll");
   DEBUG ("dpu: module saved to 'out.ll'");

   // prepare arguments for the program
   e.argv.push_back ("prog");
   e.envp.push_back ("HOME=/home/cesar");
   e.envp.push_back (nullptr);

   e.run ();
   DEBUG ("dpu: exitcode %d", e.exitcode);
   return;
}

void dpu::fe3::test ()
{
   fe3_test3 ();
   fflush (stdout);
   llvm::outs().flush ();
   llvm::errs().flush ();
}

