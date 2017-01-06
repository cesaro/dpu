
#ifndef _C15U_C15UNFOLDER_HH_
#define _C15U_C15UNFOLDER_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#undef DEBUG // exported by <llvm/ExecutionEngine/ExecutionEngine.h>

#include "pes/cut.hh"
#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/process.hh"
#include "pes/unfolding.hh"

#include "c15u/disset.hh"
#include "c15u/trail.hh"

namespace dpu
{

class C15unfolder
{
public:
   Unfolding u;

   // dynamic executor
   std::string path;
   llvm::Module *m;
   Executor *exec;

   // ctor and dtor
   C15unfolder ();
   ~C15unfolder ();

   /// load the llvm module from the "path" file
   void load_bytecode (std::string &&filepath);

   //void set_env (std::vector<const std::string> env);
   void set_args (std::vector<const char *> argv);

   /// runs the system up to completion (termination) using the provided replay
   /// and returns the corresponding maximal configuration
   Config add_one_run (const std::vector<int> &replay);

   /// runs the system up to completion using the replay, computes CEX of the
   /// resulting configuration, constructs a replay for each one of them and
   /// applies add_one_run for each one
   void add_multiple_runs (const std::vector<int> &replay);

   /// the CONCUR'15 POR algorithm
   void explore ();

   /// compute the conflicting extensions of c and add them to a singly-linked
   /// list pointed by head
   void compute_cex (Config &c, Event **head);

   /// returns false only if no alternative to D \cup {e} after C exists
   bool might_find_alternative (Config &c, Disset &d, Event *e);


   bool is_conflict_free(std::vector<Event *> eset);

//   bool find_alternative (Config &c, std::vector<Event*> d, Cut &J);
   void enumerate_combination (unsigned i, std::vector<std::vector<Event *>> comb,
         std::vector<Event*> temp, Cut &J);
   bool find_alternative (Config &c, std::vector<Event*> d, Cut &j);


   /// finds alternatives to D after C; complete but unoptimal
   bool find_alternative_only_last (const Config &c, Disset &d, Cut &j);

public:
   std::vector<std::string> argv;

   /// translates the stream of actions into events, updating c, t, and d
   inline void stream_to_events
         (Config &c,
         const action_streamt &s,
         Trail *t = nullptr,
         Disset *d = nullptr);

   /// receives a stream, an iterator to that stream, a vector mapping stream
   /// pids to our pids, and a trail; this method advances the iterator of the
   /// stream asserting that the actions found match those of
   /// the trail; the iterator is left pointing at one plus the (blue) action
   /// matched with the last event in the trail; it also updates the pidmap at
   /// thread-creation events
   inline void __stream_match_trail
         (const action_streamt &s,
         action_stream_itt &it,
         std::vector<unsigned> &pidmap,
         Trail &t);

   /// extends the replay vector with a replay sequence for the configuration c
   void cut_to_replay (const Cut &c, std::vector<int> &replay);

   /// extends the replay vector with a replay sequence for the events in
   /// c1 \setminus c2; it assumes that c1 \cup c2 is a configuration
   void cut_to_replay (const Cut &c1, const Cut &c2, std::vector<int> &replay);

   /// stores in the replay vector a suitable replay sequence for C \cup J
   void alt_to_replay (const Cut &c, const Cut &j, std::vector<int> &replay);

   /// computes conflicting extensions associated to event e
   void compute_cex_lock (Event *e, Event **head);
};

// implementation of inline methods
#include "c15u/c15unfold.hpp"

} //end of namespace
#endif

