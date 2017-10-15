#ifndef _UNFOLDER_HH_
#define _UNFOLDER_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/unfolding.hh"

#include "unfolder/replay.hh"
#include "unfolder/stream-converter.hh"

#include "defectreport.hh"

#include "unfolder/redbox.hh" // FIXME: remove

namespace dpu
{

class Unfolder;

template<>
struct StreamConverterTraits<Unfolder>
{
   /// The list of all defects found during the exploration
   Defectreport report;
};

class Unfolder : public StreamConverter<Unfolder>
{
public:

   /// A Steroids dynamic executor
   stid::Executor *exec;

   // ctor and dtor
   Unfolder (stid::ExecutorConfig &&config);
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

   /// runs the system up to completion (termination) using the provided replay
   /// and returns the corresponding maximal configuration
   Config add_one_run (const Replay &r);

   /// determines if the causal closure of all events in eset is a configuration
   bool is_conflict_free(const std::vector<Event *> &sol, const Event *e) const;

protected:

   /// Initializes the fields of a Defectreport with the parameters of this
   /// C15unfolder. This method can be overloaded in subclasses. Those overloadings
   /// will, probably call to this implementation to fill the report fields with
   /// the parmeters stored in this class, and then add others possibly stored
   /// in the subclasses.
   void report_init ();

   /// FIXME
   inline void report_add_nondet_violation (const Trail &t, unsigned where,
      ActionType found);

   /// File name for the llvm module under analysis
   std::string path;

   /// LLVM module under analysis
   llvm::Module *m;

   /// Configuration for the dynamic executor in Steroids
   stid::ExecutorConfig config;
};

// implementation of inline methods
#include "unfolder/unfolder.hpp"

} //end of namespace
#endif
