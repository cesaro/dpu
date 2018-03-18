#ifndef _UNFOLDER_HH_
#define _UNFOLDER_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/unfolding.hh"

#include "unfolder/replay.hh"
#include "unfolder/stream-converter.hh"

//#include "defectreport.hh" // FIXME: remove
//#include "redbox-factory.hh" // FIXME: remove

namespace dpu
{

template<typename T = void>
class Unfolder : public StreamConverter<T>
{
public:

   /// A Steroids dynamic executor
   stid::Executor *exec;

   // ctor and dtor
   Unfolder (const stid::ExecutorConfig &config);
   virtual ~Unfolder ();

   /// Load the llvm module from the file \p filepath
   void load_bitcode (std::string &&filepath);

   /// Saves the llvm module to a file
   void store_bitcode (const std::string &filename) const;

   /// List all external symbols in the lodaded bytecode
   void print_external_syms (const char *prefix);

   /// Sets the argv vector of the program to verify
   void set_args (std::vector<const char *> argv);
      
   /// Sets the environment variables of the program to verify
   void set_env (std::vector<const char *> env);

   /// Sets the environment variables of the program to verify to be a copy of
   /// our own environment, see environ(7)
   void set_default_environment ();

   /// Runs the system up to completion (termination) using the provided replay
   /// and returns the corresponding maximal configuration. FIXME: the replay
   /// should be a dpu::Replay, but I temporarily reverted it to a stid::Replay
   /// to get something working ...
   Config add_one_run (const stid::Replay &r);

   /// determines if the causal closure of all events in eset is a configuration
   bool is_conflict_free(const std::vector<Event *> &sol, const Event *e) const;

protected:

   /// A context object where LLVM will store the types and constants
   llvm::LLVMContext context;

   /// File name for the llvm module under analysis
   std::string path;

   /// LLVM module under analysis
   llvm::Module *m;

   /// Configuration for the dynamic executor in Steroids
   stid::ExecutorConfig config;
};

} //end of namespace

// implementation of inline methods, outside of the namespace
#include "unfolder/unfolder.hpp"

#endif
