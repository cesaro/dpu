
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

#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Format.h>

#include "stid/executor.hh"
#include "pes/process.hh"
#include "unfolder/unfolder.hh" // must be before verbosity.h

#include "misc.hh"
#include "verbosity.h"
#include "opts.hh"

namespace dpu
{

template<typename T>
Unfolder<T>::Unfolder (const stid::ExecutorConfig &config) :
   StreamConverter<T> (),
   exec (nullptr),
   m (nullptr),
   config (config)
{
   unsigned i;

   // initialize the start array (this is an invariant expected and maintained
   // by stream_to_events)
   for (i = 0; i < Unfolding::MAX_PROC; i++)
      StreamConverter<T>::start[i] = nullptr;
}

template<typename T>
Unfolder<T>::~Unfolder ()
{
   DEBUG ("unf.dtor: this %p", this);
   delete exec;
}

template<typename T>
void Unfolder<T>::load_bitcode (std::string &&filepath)
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

   // parse the .ll file and get a Module out of it
   PRINT ("dpu: unf: loading bitcode");
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (path, err, context));
   m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (path.c_str(), os);
      os.flush ();
      DEBUG ("unf: unf: load-bytecode: '%s': %s\n", path.c_str(), errors.c_str());
      throw std::invalid_argument (errors);
   }

   // print external symbols
   if (verb_trace) print_external_syms ("dpu: ");

   PRINT ("dpu: unf: O%u-optimization + jitting...", opts::optlevel);
   DEBUG ("unf: unf: load-bytecode: setting up the bytecode executor...");
   try {
      exec = new stid::Executor (std::move (mod), config);
   } catch (const std::exception &e) {
      DEBUG ("unf: unf: load-bytecode: errors preparing the bytecode executor");
      DEBUG ("unf: unf: load-bytecode: %s", e.what());
      throw e;
   }
   DEBUG ("unf: unf: load-bytecode: executor successfully created!");

   if (opts::instpath.size())
   {
      TRACE ("dpu: unf: load-bytecode: saving instrumented bytecode to %s",
         opts::instpath.c_str());
      store_bitcode (opts::instpath.c_str());
   }

   DEBUG ("dpu: unf: load-bytecode: done!");
}

template<typename T>
void Unfolder<T>::store_bitcode (const std::string &filename) const
{
   int fd = open (filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *m;
}

template<typename T>
void Unfolder<T>::print_external_syms (const char *prefix)
{
   std::string str;
   llvm::raw_string_ostream os (str);
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> funs;
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> globs;
   size_t len;

   ASSERT (m);
   if (! prefix) prefix = "";

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
      os << prefix << llvm::format ("  %-*s ", len, pair.first.str().c_str());
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
      os << prefix << llvm::format ("  %-*s ", len, pair.first.str().c_str());
      os << *pair.second << "\n";
   }
   os.flush ();
   PRINT ("%s", str.c_str());
}

template<typename T>
void Unfolder<T>::set_args (std::vector<const char *> argv)
{
   DEBUG ("unf: set-args: |argv| %zu", argv.size());
   exec->argv = argv;
}

template<typename T>
void Unfolder<T>::set_env (std::vector<const char *> env)
{
   if (env.empty() or env.back() != nullptr)
      env.push_back (nullptr);
   DEBUG ("unf: set-env: |env| %zu", env.size());
   exec->environ = env;
}

template<typename T>
void Unfolder<T>::set_default_environment ()
{
   char * const * v;
   std::vector<const char *> env;

   // make a copy of our environment
   for (v = environ; *v; ++v) env.push_back (*v);
   env.push_back (nullptr);
   DEBUG ("unf: set-env: |env| %zu", env.size());
   exec->environ = env;
}

template<typename T>
Config Unfolder<T>::add_one_run (const stid::Replay &r)
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
   //actions.print ();
   StreamConverter<T>::convert (actions, c);
   return c;
}

template<typename T>
bool Unfolder<T>::is_conflict_free (const std::vector<Event *> &sol,
   const Event *e) const
{
   // we return false iff e is in conflict with some event of the partial solution "sol"
   int i;
   for (i = 0; i < sol.size(); i++)
      if (e->in_cfl_with (sol[i])) return false;
   return true;
}

} // namespace dpu
