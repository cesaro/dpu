#ifndef _UNFOLDER_HH_
#define _UNFOLDER_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#include "pes/cut.hh"
#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/process.hh"
#include "pes/unfolding.hh"

#include "unfolder/disset.hh"
#include "unfolder/trail.hh"
#include "unfolder/pidpool.hh"
#include "unfolder/replay.hh"

#include "defectreport.hh"

namespace dpu
{

class Unfolder
{
public:

   /// the unfolding data structure
   Unfolding u;

   /// The list of all defects found during the exploration
   Defectreport report;

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

   /// Translates the stream of actions into events, updating c, t, and d
   inline bool stream_to_events (Config &c, const stid::action_streamt &s,
         Trail *t = nullptr, Disset *d = nullptr);

   /// Receives a stream, an iterator to that stream, a vector mapping stream
   /// pids to our pids, and a trail; this method advances the iterator of the
   /// stream asserting that the actions found match those of
   /// the trail; the iterator is left pointing at one plus the (blue) action
   /// matched with the last event in the trail; it also updates the pidmap at
   /// thread-creation events
   inline bool stream_match_trail (const stid::action_streamt &s,
         stid::action_stream_itt &it, Trail &t, Pidmap &pidmap);

   /// Initializes the fields of a Defectreport with the parameters of this
   /// C15unfolder. This method can be overloaded in subclasses. Those overloadings
   /// will, probably call to this implementation to fill the report fields with
   /// the parmeters stored in this class, and then add others possibly stored
   /// in the subclasses.
   void report_init (Defectreport &r) const;

protected:

   /// FIXME
   inline void report_add_nondet_violation (const Trail &t, unsigned where,
      ActionType found);

   /// File name for the llvm module under analysis
   std::string path;

   /// LLVM module under analysis
   llvm::Module *m;

   /// Configuration for the dynamic executor in Steroids
   stid::ExecutorConfig config;

   /// This object is used to select the pid of a process whenver we create a
   /// new THCREAT event
   Pidpool pidpool;

   /// The method tream_match_trail() needs to communicate to
   /// stream_to_events() the THSTART events of every thread whose THCREAT was
   /// parsed in stream_match_trail() but whose THSTART didn't make it to the
   /// configuration before stream_to_events() got the control. We use this map
   /// to achieve that, we map each pid to the THSTART event in that thread for
   /// this execution.
   Event *start[Unfolding::MAX_PROC];
};

// implementation of inline methods
#include "unfolder/unfolder.hpp"

} //end of namespace
#endif


