#ifndef __PES_UNFOLDING_HH_
#define __PES_UNFOLDING_HH_

#include <fstream>
#include <unordered_map>
#include <stdlib.h>

#include "pes/event.hh"

namespace dpu
{

class Process;

class Unfolding : public UnfoldingMemoryMath
{
public:
   Unfolding ();
   inline ~Unfolding ();
   Unfolding (const Unfolding &other) = delete; // no copy constructor
   inline Unfolding (const Unfolding &&other);

   void dump () const;
   void print_dot (std::ofstream &fs, unsigned col = 0, std::string &&msg = "");
   void print_dot (const std::string &path, unsigned col = 0,
         std::string &&msg = "");
   void print_dot (Cut &c, std::ofstream &fs, std::string &&msg = "");

   /// returns a pointer to the process number pid
   inline Process *proc (unsigned pid) const;

   /// returns the number of processes currently present in this unfolding
   unsigned num_procs () const { return nrp; }

   /// The following methods create or retrieve existing events from the unfolding
   /// THSTART(), creat is the corresponding THCREAT (or null for p0);
   /// this will create a new process if a new event needs to be created
   Event *event (Event *creat);
   /// THCREAT(tid) or THEXIT(), one predecessor (in the process)
   Event *event (Action ac, Event *p);
   /// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
   Event *event (Action ac, Event *p, Event *m);

   /// returns a new fresh color
   inline unsigned get_fresh_color ();

private :
   /// memory space where Processes, EventBoxes and Events will be stored
   char *procs;
   /// number of processes created using new_proc (<= MAX_PROC)
   unsigned nrp;
   /// a fresh color, used for Events or anywhere else
   unsigned color;
   /// map from lock addresss to events, necessary to insert into a circular
   /// list the roots of the MTXLOCK events
   std::unordered_map<Addr,Event*> lockroots;

   /// creates a new process in the unfolding; creat is the corrsponding THCREAT
   /// event
   inline Process *new_proc (Event *creat);

   /// returns the (only) immediate causal successor of c (which is a THCREAT)
   /// if it exists, or NULL if it doesn't
   Event *find0 (Event *c);
   /// returns the (only) immediate process causal successor of p with action *ac, or NULL
   inline Event *find1 (Action *ac, Event *p);
   /// returns the (only) immediate causal successor of {p,m} with action *ac, or NULL
   inline Event *find2 (Action *ac, Event *p, Event *m);
};

// implementation of inline methods
#include "pes/unfolding.hpp"

} // namespace dpu
#endif
