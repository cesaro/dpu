
#ifndef _C15UNFOLD_HH_
#define _C15UNFOLD_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#include "pes.hh"

namespace dpu
{

class Disset
{
public:
   Disset ();
   typedef struct {
      Event *e;
      unsigned idx;
      unsigned disabler;
      /// next element in the list of justified or unjustified events
      Elem *next;
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

class Trail : std:vector<Event*>
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
   Dissset d;

   // dynamic executor
   llvm::Module *m;
   Executor *exec;
   std::vector<std::string> argv;


   C15unfolder ();

   /// load the llvm module from the "path" file
   void load_bytecode (const char *path);

   /// the POR algorithm
   void explore ();

private:
   void stream_to_events (action_streamt &s, BaseConfig &c);
   conf_to_replay (C, rep)
   find_cex (C)
};

} //end of namespace
#endif
