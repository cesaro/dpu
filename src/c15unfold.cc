
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstdint>
#include <cstring>
#include <stdlib.h>
#include <cstdio>
#include <string>
#include <algorithm>

//#include "llvm/Pass.h"
//#include "llvm/PassSupport.h"
#include "llvm/IR/Module.h"
//#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
//#include "llvm/IR/IRBuilder.h"
//#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/InstIterator.h"
//#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
//#include "llvm/ExecutionEngine/ExecutionEngine.h"
//#include "llvm/ExecutionEngine/SectionMemoryManager.h"
//#include "llvm/ExecutionEngine/MCJIT.h"

#include "c15unfold.hh"
#include "pes.hh"
#include "misc.hh"
#include "verbosity.h"

namespace dpu
{

static void _ir_write_ll (const llvm::Module *m, const char *filename)
{
   int fd = open (filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *m;
}


Disset::Disset ()
{
}

C15unfolder::C15unfolder () :
   m (nullptr),
   exec (nullptr)
{
}

C15unfolder::~C15unfolder ()
{
   DEBUG ("c15u.dtor: this %p", this);
   delete exec;
}

void C15unfolder::load_bytecode (std::string &&filepath)
{
   llvm::SMDiagnostic err;
   std::string errors;

   DEBUG ("c15u: load-bytecode: this %p path '%s'", this, filepath.c_str());

   ASSERT (filepath.size());
   ASSERT (path.size() == 0);
   ASSERT (exec == 0);
   ASSERT (m == 0);
   ASSERT (argv.size() == 0);
   path = std::move (filepath);

   // related to the JIT engine
   static bool init = false;
   if (not init)
   {
      init = true;
      llvm::InitializeNativeTarget();
      llvm::InitializeNativeTargetAsmPrinter();
      llvm::InitializeNativeTargetAsmParser();
   }

   // get a context
   llvm::LLVMContext &context = llvm::getGlobalContext();

   // parse the .ll file and get a Module out of it
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (path, err, context));
   m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (path.c_str(), os);
      os.flush ();
      DEBUG ("c51u: load-bytecode: '%s': %s\n", path.c_str(), errors.c_str());
      throw std::invalid_argument (errors);
   }

   // prepare an Executor, the constructor instruments and allocates guest
   // memory
   // FIXME - make this configurable via methods of the C15unfolder
   ExecutorConfig conf;
   conf.memsize = 128 << 20;
   conf.stacksize = 16 << 20;
   conf.tracesize = 16 << 20;

   DEBUG ("c15u: load-bytecode: creating a bytecode executor...");
   try {
      exec = new Executor (std::move (mod), conf);
   } catch (const std::exception &e) {
      DEBUG ("c15u: load-bytecode: errors preparing the bytecode executor");
      DEBUG ("c15u: load-bytecode: %s", e.what());
      throw e;
   }
   DEBUG ("c15u: load-bytecode: executor successfully created!");

   DEBUG ("c15u: load-bytecode: saving instrumented code to /tmp/output.ll");
   _ir_write_ll (m, "/tmp/output.ll");

   DEBUG ("c15u: load-bytecode: done!");
}

void C15unfolder::set_args (std::vector<const char *> argv)
{
   DEBUG ("c15u: set-args: |argv| %d", argv.size());

   // FIXME - this should be moved to a proper API
   exec->envp.push_back ("HOME=/home/cesar/");
   exec->envp.push_back ("PWD=/usr/bin");
   exec->envp.push_back (nullptr);

   exec->argv = argv;
}

Config C15unfolder::add_one_run (std::vector<int> &replay)
{
   Config c (Unfolding::MAX_PROC);

   ASSERT (exec);

   // run the guest
   DEBUG ("c15u: add-1-run: this %p |replay| %d", this, replay.size());
   exec->set_replay (replay.data(), replay.size());
   DEBUG ("c15u: add-1-run: running the guest ...");
   exec->run ();

   // get a stream object from the executor and transform it into events
   action_streamt actions (exec->get_trace ());
   actions.print ();
   stream_to_events (c, actions);
   return c;
}

void C15unfolder::run_to_completion (Config &c)
{
}

void C15unfolder::explore ()
{
}

void C15unfolder::stream_to_events (Config &c, action_streamt &s)
{
   // invariant:
   // the stream is a sequence of actions starting from the state given by c

   Event *e, *ee;
   action_stream_itt it (s.begin());
   action_stream_itt end (s.end());
   std::vector<int> pidmap (s.get_rt()->trace.num_ths);

   // we should have at least one action in the stream
   ASSERT (it != end);

   // skip the first context switch to pid 0, if present
   if (it.type () == RT_THCTXSW)
   {
      ASSERT (it.has_id ());
      ASSERT (it.id() == 0);
      it++;
   }

   // for the time being, we assume that the configuration is totally pristine
   ASSERT (c[0] == 0);
   e = u.event (nullptr); // bottom
   c.add (e);
   pidmap[0] = 0;

   while (it != end)
   {
      switch (it.type())
      {
      case RT_MTXLOCK :
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXLOCK, .addr = it.addr()}, e, ee);
         c.add (e);
         break;

      case RT_MTXUNLK :
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXUNLK, .addr = it.addr()}, e, ee);
         c.add (e);
         break;

      case RT_THCTXSW :
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() == 0 or pidmap[it.id()] != 0); // map is defined
         e = c[pidmap[it.id()]];
         ASSERT (e); // we have at least one event there
         break;

      case RT_THCREAT :
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() >= 1 and pidmap[it.id()] == 0); // map entry undefined
         // creat
         e = u.event ({.type = ActionType::THCREAT, .addr = it.addr()}, e);
         // start
         ee = u.event (e);
         e->action.val = ee->pid();
         pidmap[it.id()] = ee->pid();
         c.add (e);
         c.add (ee);
         break;

      case RT_THEXIT :
         e = u.event ({.type = ActionType::THEXIT}, e);
         c.add (e);
         break;

      case RT_THJOIN :
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() >= 1 and pidmap[it.id()] != 0); // map is defined
         ee = c.mutex_max (pidmap[it.id()]);
         ASSERT (ee and ee->action.type == ActionType::THEXIT);
         e = u.event ({.type = ActionType::THJOIN, .val = it.id()}, e, ee);
         c.add (e);
         break;

      case RT_RD8 :
         e->redbox.push_back
               ({.type = ActionType::RD8, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD16 :
         e->redbox.push_back
               ({.type = ActionType::RD16, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD32 :
         e->redbox.push_back
               ({.type = ActionType::RD32, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD64 :
         e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD128 :
         ASSERT (it.val_size() == 2);
         e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr(), .val = it.val()[0]});
         e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr()+8, .val = it.val()[1]});
         break;

      case RT_WR8 :
         e->redbox.push_back
               ({.type = ActionType::WR8, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR16 :
         e->redbox.push_back
               ({.type = ActionType::WR16, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR32 :
         e->redbox.push_back
               ({.type = ActionType::WR32, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR64 :
         e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR128 :
         ASSERT (it.val_size() == 2);
         e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr(), .val = it.val()[0]});
         e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr()+8, .val = it.val()[1]});
         break;

      case RT_ALLOCA :
      case RT_MALLOC :
      case RT_FREE :
      case RT_CALL :
      case RT_RET :
         // not implemented, but doesn't hurt ;)
         break;

      default :
         SHOW (it.type(), "d");
         SHOW (it.addr(), "lu");
         SHOW (it.val()[0], "lu");
         SHOW (it.id(), "u");
         SHOW (it.str(), "s");
         ASSERT (0);
      }

      // get the next action
      it++;
   }
}

void C15unfolder::conf_to_replay (Cut &c, std::vector<int> &replay)
{
}

void C15unfolder::compute_cex (Config &c)
{
}

} // namespace dpu
