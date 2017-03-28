
#ifndef _C15U_C15UNFOLDER_HH_
#define _C15U_C15UNFOLDER_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#include "pes/cut.hh"
#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/process.hh"
#include "pes/unfolding.hh"

#include "c15u/disset.hh"
#include "c15u/trail.hh"
#include "c15u/pidpool.hh"
#include "c15u/replay.hh"

#include "defectreport.hh"

namespace dpu
{

class C15unfolder
{
public:
   typedef enum { SDPOR, ONLYLAST, KPARTIAL, OPTIMAL } Alt_algorithm;

   /// the unfolding data structure
   Unfolding u;

   /// The list of all defects found during the exploration
   Defectreport report;

   /// counters to obtain statistics
   struct {
      long unsigned runs = 0;
      long unsigned ssbs = 0;
      long unsigned maxconfs = 0;
      long unsigned stid_threads = 0;
      float avg_max_trail_size = 0;
      struct {
         long unsigned calls = 0; // all calls
         long unsigned calls_built_comb = 0; // had to build the comb
         long unsigned calls_explore_comb = 0; // got to explore the comb
         Probdist<unsigned> spikes; // number of spikes
#ifdef CONFIG_STATS_DETAILED
         Probdist<unsigned> spikesize; // size of the spikes
#endif
      } alt;
   } counters;

   // dynamic executor
   std::string path;
   llvm::Module *m;
   stid::Executor *exec;

   // ctor and dtor
   C15unfolder (Alt_algorithm a, unsigned kbound);
   ~C15unfolder ();

   /// load the llvm module from the "path" file
   void load_bytecode (std::string &&filepath);

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

   /// runs the system up to completion using the replay, computes CEX of the
   /// resulting configuration, constructs a replay for each one of them and
   /// applies add_one_run for each one
   void add_multiple_runs (const Replay &r);

   /// the CONCUR'15 POR algorithm
   void explore ();

   /// compute the conflicting extensions of c and add them to a singly-linked
   /// list pointed by head
   void compute_cex (Config &c, Event **head);

   /// determines if the causal closure of all events in eset is a configuration
   bool is_conflict_free(const std::vector<Event *> &sol, const Event *e) const;

   /// recursive function to explore all combinations in the comb of
   /// alternatives
   bool enumerate_combination (unsigned i, std::vector<std::vector<Event *>> &comb,
         std::vector<Event*> &sol);

   /// returns false only if no alternative to D \cup {e} after C exists
   bool might_find_alternative (Config &c, Disset &d, Event *e);

   /// finds one alternative for C after D, and stores it in J; we select from
   /// here the specific algorithm that we cal
   bool find_alternative (const Trail &t, Config &c, const Disset &d, Cut &j);

   /// Implementation 1: complete, unoptimal, searches conflict to only last event
   bool find_alternative_only_last (const Config &c, const Disset &d, Cut &j);

   /// Implementation 2: complete, optimal/unoptimal, based on the comb
   bool find_alternative_kpartial (const Config &c, const Disset &d, Cut &j);

   /// Implementation 2: complete, unoptimal
   bool find_alternative_sdpor (Config &c, const Disset &d, Cut &j);

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

   void set_replay_and_sleepset (Replay &replay, const Cut &j, const Disset &d);

   /// Computes conflicting extensions associated to event e
   void compute_cex_lock (Event *e, Event **head);

private:
   std::string explore_stat (const Trail &t, const Disset &d) const;
   void report_init (Defectreport &r) const;
   inline void report_add_nondet_violation (const Trail &t, unsigned where, ActionType found);

   Alt_algorithm alt_algorithm;
   unsigned kpartial_bound;

   /// This object is used to select the pid of a process whenver we create a
   /// new THCREAT event
   Pidpool pidpool;

   /// The method __stream_match_trail() needs to communicate to
   /// stream_to_events() the THSTART events of every thread whose THCREAT was
   /// parsed in __stream_match_trail() but whose THSTART didn't make it to the
   /// configuration before stream_to_events() got the control. We use this map
   /// to achieve that, we map each pid to the THSTART event in that thread for
   /// this execution.
   Event *start[Unfolding::MAX_PROC];
};

// implementation of inline methods
#include "c15u/c15unfold.hpp"

} //end of namespace
#endif

