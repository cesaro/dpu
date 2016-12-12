
#ifndef _C15UNFOLD_HH_
#define _C15UNFOLD_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#undef DEBUG // exported by <llvm/ExecutionEngine/ExecutionEngine.h>

#include "pes.hh"

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
   std::vector<std::string> argv;


   C15unfolder ();
   ~C15unfolder ();

   /// load the llvm module from the "path" file
   void load_bytecode (std::string &&filepath);

   /// runs the system and returns the generated configuration
   Config add_one_run (std::vector<int> &replay);
   void run_to_completion (Config &c);

   /// the POR algorithm
   void explore ();
   void compute_cex (Unfolding &u, Config &c);
   bool find_alternative (Config &c, std::vector<Event*> d, Config &j);

private:
   void stream_to_events (Config &c, action_streamt &s);
   void conf_to_replay (Cut &c, std::vector<int> &replay);
//   void compute_cex (Unfolding &u, Config &c);
};

} //end of namespace
#endif
