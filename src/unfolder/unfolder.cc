
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <algorithm>

#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include <llvm/Support/YAMLTraits.h>
#include <llvm/Support/Format.h>

#include "stid/executor.hh"
#include "pes/process.hh"
#include "unfolder/unfolder.hh" // must be before verbosity.h

#include "misc.hh"
#include "verbosity.h"
#include "opts.hh"

namespace dpu
{

Unfolder::Unfolder (stid::ExecutorConfig &&config) :
   u (),
   report (),
   exec (nullptr),
   m (nullptr),
   config (std::move (config)),
   pidpool (u)
{
   unsigned i;

   // initialize the start array (this is an invariant expected and maintained
   // by stream_to_events)
   for (i = 0; i < Unfolding::MAX_PROC; i++) start[i] = nullptr;
}

Unfolder::~Unfolder ()
{
   DEBUG ("unf.dtor: this %p", this);
   delete exec;
}

void Unfolder::load_bitcode (std::string &&filepath)
{
   llvm::SMDiagnostic err;
   std::string errors;

   ASSERT (filepath.size());
   ASSERT (path.size() == 0);
   ASSERT (exec == 0);
   ASSERT (m == 0);
   path = std::move (filepath);

   // necessary for the JIT engine; we should move this elsewhere
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
   PRINT ("dpu: loading bitcode...");
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (path, err, context));
   m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (path.c_str(), os);
      os.flush ();
      DEBUG ("unf: load-bytecode: '%s': %s\n", path.c_str(), errors.c_str());
      throw std::invalid_argument (errors);
   }

   // print external symbols
   if (verb_trace) print_external_syms ("dpu: ");

   PRINT ("dpu: O%u-optimization + jitting...", opts::optlevel);
   DEBUG ("unf: load-bytecode: setting up the bytecode executor...");
   try {
      exec = new stid::Executor (std::move (mod), config);
   } catch (const std::exception &e) {
      DEBUG ("unf: load-bytecode: errors preparing the bytecode executor");
      DEBUG ("unf: load-bytecode: %s", e.what());
      throw e;
   }
   DEBUG ("unf: load-bytecode: executor successfully created!");

   if (opts::instpath.size())
   {
      TRACE ("unf: load-bytecode: saving instrumented bytecode to %s", opts::instpath.c_str());
      store_bitcode (opts::instpath.c_str());
   }

   DEBUG ("unf: load-bytecode: done!");
}

void Unfolder::store_bitcode (const std::string &filename) const
{
   int fd = open (filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *m;
}

void Unfolder::print_external_syms (const char *prefix)
{
   std::string str;
   llvm::raw_string_ostream os (str);
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> funs;
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> globs;
   size_t len;

   ASSERT (m);

   // functions
   len = 0;
   for (llvm::Function &f : m->functions())
   {
      if (not f.isDeclaration()) continue;
      funs.emplace_back (f.getName(), f.getType());
      if (len < funs.back().first.size())
         len = funs.back().first.size();
   }
   std::sort (funs.begin(), funs.end());
   os << prefix << "External functions:\n";
   for (auto &pair : funs)
   {
      os << prefix << llvm::format ("  %-*s ", len, pair.first);
      os << *pair.second << "\n";
   }

   // global variables
   len = 0;
   for (llvm::GlobalVariable &g : m->globals())
   {
      if (not g.isDeclaration()) continue;
      globs.emplace_back (g.getName(), g.getType());
      if (len < globs.back().first.size())
         len = globs.back().first.size();
   }
   std::sort (globs.begin(), globs.end());
   os << prefix << "External variables:\n";
   for (auto &pair : globs)
   {
      os << prefix << llvm::format ("  %-*s ", len, pair.first);
      os << *pair.second << "\n";
   }
   os.flush ();
   PRINT ("%s", str.c_str());
}

void Unfolder::set_args (std::vector<const char *> argv)
{
   DEBUG ("unf: set-args: |argv| %zu", argv.size());
   exec->argv = argv;
}

void Unfolder::set_env (std::vector<const char *> env)
{
   if (env.empty() or env.back() != nullptr)
      env.push_back (nullptr);
   DEBUG ("unf: set-env: |env| %zu", env.size());
   exec->environ = env;
}

void Unfolder::set_default_environment ()
{
   char * const * v;
   std::vector<const char *> env;

   // make a copy of our environment
   for (v = environ; *v; ++v) env.push_back (*v);
   env.push_back (nullptr);
   DEBUG ("unf: set-env: |env| %zu", env.size());
   exec->environ = env;
}

Config Unfolder::add_one_run (const Replay &r)
{
   Config c (Unfolding::MAX_PROC);

   ASSERT (exec);

   // run the guest
   DEBUG ("unf: add-1-run: this %p |replay| %zu", this, r.size());
   exec->set_replay (r);
   DEBUG ("unf: add-1-run: running the guest ...");
   exec->run ();

   // get a stream object from the executor and transform it into events
   stid::action_streamt actions (exec->get_trace ());
   actions.print ();
   stream_to_events (c, actions);
   return c;
}

bool Unfolder::is_conflict_free (const std::vector<Event *> &sol,
   const Event *e) const
{
   // we return false iff e is in conflict with some event of the partial solution "sol"
   int i;
   for (i = 0; i < sol.size(); i++)
      if (e->in_cfl_with (sol[i])) return false;
   return true;
}

void Unfolder::report_init ()
{
   ASSERT (exec);
   std::vector<std::string> myargv (exec->argv.begin(), exec->argv.end());
   std::vector<std::string> myenv (exec->environ.begin(), --(exec->environ.end()));

   report.dpuversion = CONFIG_VERSION;
   report.path = path;
   report.argv = myargv;
   report.environ = myenv;
   report.memsize = exec->config.memsize;
   report.defaultstacksize = exec->config.defaultstacksize;
   report.tracesize = exec->config.tracesize;
   report.optlevel = exec->config.optlevel;

   report.nr_exitnz = 0;
   report.nr_abort = 0;
   report.defects.clear ();
}

} // namespace dpu
