
#ifndef _C15U_C15UNFOLDER_HH_
#define _C15U_C15UNFOLDER_HH_

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
#include "unfolder/unfolder.hh"

#include "defectreport.hh"

namespace dpu
{
typedef enum { SDPOR, ONLYLAST, KPARTIAL, OPTIMAL } Altalgo;
};

// requires Altalgo to be defined
#include "unfolder/comb.hh"

namespace dpu
{

class C15unfolder : public Unfolder
{
public:
   /// counters to obtain statistics
   struct {
      bool timeout = false;
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
         Probdist<unsigned> spikesizeunfilt; // size of the spikes before filtering out
         Probdist<unsigned> spikesizefilt; // size of the spikes after filtering out
#endif
      } alt;
   } counters;

   // ctor and dtor
   C15unfolder (Altalgo a, unsigned kbound, unsigned maxcts);
   virtual ~C15unfolder ();

   /// Initialize and return the parameters of a stid::Executor
   stid::ExecutorConfig prepare_executor_config () const;

   /// runs the system up to completion using the replay, computes CEX of the
   /// resulting configuration, constructs a replay for each one of them and
   /// applies add_one_run for each one
   void add_multiple_runs (const Replay &r);

   /// the CONCUR'15 POR algorithm
   void explore ();

   /// compute the conflicting extensions of c and add them to a singly-linked
   /// list pointed by head
   void compute_cex (Config &c, Event **head);

   /// recursive function to explore all combinations in the comb of
   /// alternatives
   bool enumerate_combination (unsigned i, std::vector<Event*> &sol);

   /// returns false only if no alternative to D \cup {e} after C exists
   bool might_find_alternative (Config &c, Disset &d, Event *e);

   /// finds one alternative for C after D, and stores it in J; we select from
   /// here the specific algorithm that we call
   bool find_alternative (const Trail &t, Config &c, const Disset &d, Cut &j);

   /// Implementation 1: complete, unoptimal, searches conflict to only last event
   bool find_alternative_only_last (const Config &c, const Disset &d, Cut &j);

   /// Implementation 2: complete, optimal/unoptimal, based on the comb
   bool find_alternative_kpartial (const Config &c, const Disset &d, Cut &j);

   /// Implementation 2: complete, unoptimal
   bool find_alternative_sdpor (Config &c, const Disset &d, Cut &j);

   /// FIXME
   void set_replay_and_sleepset (Replay &replay, const Cut &j, const Disset &d);

   /// Computes conflicting extensions associated to event e
   void compute_cex_lock (Event *e, Event **head);

   /// Initializes the fields of a Defectreport with the parameters of this
   /// C15unfolder.
   virtual void report_init (Defectreport &r) const;

private:
   /// Returns debugging output suitable to be printed
   std::string explore_stat (const Trail &t, const Disset &d) const;

   /// Algorithm to compute alternatives
   Altalgo altalgo;

   /// When computing k-partial alternatives, the value of k
   unsigned kpartial_bound;

   /// The comb data structure
   Comb comb;

   /// Maximum number of context switches present in the trail for the
   /// exploration to allow computing alternatives for an event extracted from
   /// the trail immediately before.
   unsigned max_context_switches;
};

// implementation of inline methods
//#include "c15u/c15unfold.hpp"

} //end of namespace
#endif

