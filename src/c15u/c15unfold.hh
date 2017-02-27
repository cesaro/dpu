
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
   typedef enum { SDPOR, ONLYLAST, KPARTIAL, OPTIMAL } Alt_algorithm;

   Unfolding u;
   struct {
      long unsigned runs;
      long unsigned ssbs;
      long unsigned maxconfs;
      long unsigned altcalls;
      long unsigned max_unjust_when_alt_call;
      long unsigned stid_threads;
      float avg_unjust_when_alt_call;
      float avg_max_trail_size;
   } counters;

   // dynamic executor
   std::string path;
   llvm::Module *m;
   Executor *exec;

   // ctor and dtor
   C15unfolder (Alt_algorithm a, unsigned kbound);
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

   /// implementation 1: complete, unoptimal, searches conflict to only last event
   bool find_alternative_only_last (const Config &c, const Disset &d, Cut &j);

   /// implementation 2: complete, optimal/unoptimal, based on the comb
   bool find_alternative_kpartial (const Config &c, const Disset &d, Cut &j);

   /// implementation 2: complete, unoptimal
   bool find_alternative_sdpor (Config &c, const Disset &d, Cut &j);

   /// translates the stream of actions into events, updating c, t, and d
   inline bool stream_to_events (Config &c, const action_streamt &s,
         Trail *t = nullptr, Disset *d = nullptr);

   /// receives a stream, an iterator to that stream, a vector mapping stream
   /// pids to our pids, and a trail; this method advances the iterator of the
   /// stream asserting that the actions found match those of
   /// the trail; the iterator is left pointing at one plus the (blue) action
   /// matched with the last event in the trail; it also updates the pidmap at
   /// thread-creation events
   inline void __stream_match_trail (const action_streamt &s, action_stream_itt &it,
         Trail &t);

   /// extends the replay vector with a replay sequence for the configuration c
   void cut_to_replay (const Cut &c, std::vector<int> &replay);

   /// extends the replay vector with a replay sequence for the events in
   /// c1 \setminus c2; it assumes that c1 \cup c2 is a configuration
   void cut_to_replay (const Cut &c1, const Cut &c2, std::vector<int> &replay);

   /// stores in the replay vector a suitable replay for the trail followed by
   /// J \setminus C
   void alt_to_replay (const Trail &t, const Cut &c, const Cut &j, std::vector<int> &replay);

   void set_replay_and_sleepset (std::vector<int> &replay, const Cut &j,
         const Disset &d);

   /// prints the replay into one string, marking the beginning of the replay of
   /// the alternative, after the replay for the trail finishes
   std::string replay2str (std::vector<int> &replay, unsigned altidx);

   /// extends the replay vector with a sequence suitable to replay the trail
   void trail_to_replay (const Trail &t, std::vector<int> &replay) const;

   /// computes conflicting extensions associated to event e
   void compute_cex_lock (Event *e, Event **head);

private:
   std::string explore_stat (const Trail &t, const Disset &d) const;

   /// maps thread numbers in steroids to DPU
   inline unsigned &s2dpid (unsigned stidpid);
   /// maps thread numbers in DPU to steroids
   inline unsigned &d2spid (unsigned dpupid);
   /// dump the pidmaps
   void dump_pidmaps () const;

   std::vector<std::string> argv;
   Alt_algorithm alt_algorithm;
   unsigned kpartial_bound;

   std::vector<unsigned> pidmap_s2d;
   std::vector<unsigned> pidmap_d2s;
};

// implementation of inline methods
#include "c15u/c15unfold.hpp"

} //end of namespace
#endif

