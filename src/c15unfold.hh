
#ifndef _C15UNFOLD_HH_
#define _C15UNFOLD_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#undef DEBUG // exported by <llvm/ExecutionEngine/ExecutionEngine.h>

#include "pes/cut.hh"
#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/unfolding.hh"

namespace dpu
{

class Disset
{
public:
   Disset ();
   typedef struct __elem {
      Event *e;
      unsigned idx;
      unsigned disabler;
      /// next element in the list of justified or unjustified events
      __elem *next;
   } Elem;

   /// the set D in the C'15 algorithm
   std::vector<Elem> stack;
   /// head of the singly-linked list of justified events in D
   Elem *just;
   /// head of the singly-linked list of unjustified events in D
   Elem *unjust;

   // TODO list events: in D, justified, unjustified

   /// add an event, given event and index it occupies the trail
   void add (Event *e, unsigned idx);

   /// check if an event pushed to the trail justifies one in the disset
   void trail_push (unsigned idx);

   /// check if an event poped from the trail un-justifies or removes one in the
   /// disset
   void trail_pop (unsigned idx);
};

class Trail : public std::vector<Event*>
{
   void push (Event *e)
      { std::vector<Event*>::push_back (e); }
   void pop ()
      { std::vector<Event*>::pop_back(); }
   Event * peek ()
      { return back (); }
};

class C15unfolder
{
public:
   Unfolding u;
   Trail trail;
   Disset d;

   // dynamic executor
   std::string path;
   llvm::Module *m;
   Executor *exec;


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

   /// replays c and then runs the system in free mode, adding the resulting
   /// events and updating c, which becomes a maximal configuration
   void run_to_completion (Config &c);

   /// the CONCUR'15 POR algorithm
   void explore ();

   /// compute the conflicting extensions of c and add them to a singly-linked
   /// list pointed by head
   void compute_cex (Config &c, Event **head);


//   bool find_alternative (Config &c, std::vector<Event*> d, Cut &J);

   void enumerate_combination (unsigned i, std::vector<std::vector<Event *>> comb,
         std::vector<Event*> temp, Cut &J);
   bool is_conflict_free(std::vector<Event *> eset);

   bool find_alternative (Config &c, std::vector<Event*> d, Cut &j);

   /// finds alternatives to D after C; complete but unoptimal
   bool find_alternative_only_last (Config &c, std::vector<Event*> d, Cut &j);

public:
   std::vector<std::string> argv;

   void stream_to_events (Config &c, const action_streamt &s);

   /// computes a replay sequence for the configuration c
   void cut_to_replay (const Cut &c, std::vector<int> &replay);

   /// computes conflicting extensions associated to event e
   void compute_cex_lock (Event *e, Event **head);

   /// returns true iff c \cup [e] is a configuration
   bool compatible_with (Config &c, Event &e);
};

} //end of namespace
#endif

