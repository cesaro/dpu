
#ifndef __PES_EVENT_HH_
#define __PES_EVENT_HH_

#include <vector>

#include "pes/action.hh"
#include "pes/cfltree.hh"
#include "pes/eventbox.hh"
#include "pes/primecon.hh"

#include "verbosity.h"

namespace dpu
{

class Process;

class Event : public MultiNode<Event,2> // 2 trees, skip step = 3
{
public:
   int inside; // a flag to mark that an event is inside some set or not

   /// THSTART(), creat is the corresponding THCREAT (or null for p0)
   inline Event (Event *creat);
   /// THCREAT(tid) or THEXIT(), one predecessor (in the process)
   inline Event (Action ac, bool boxfirst);
   /// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
   inline Event (Action ac, Event *m, bool boxfirst);

   struct {
      /// True iff this event is the first one in its own Eventbox
      int boxfirst : 1;
      /// True iff this event is the last one in its own Eventbox
      int boxlast : 1;
      /// True iff the redbox is completely filled
      int crb : 1;
      /// True iff this event is in the set D of the C'15 algorithm
      int ind : 1;
   } flags;

   /// the blue action performed by this event
   Action action;
   /// a list of red actions that the thread performed between Events pre() and this
   std::vector<Action> redbox;

   /// color mark for various algorithms
   unsigned color;
   /// general-purpose pointer for various algorithms
   Event *next;

   /// predecessor in my thread, or null if THSTART
   inline const Event *pre_proc () const;
   inline Event *pre_proc ();
   /// predecessor in another thread (if any), or null
   inline const Event *pre_other () const;
   inline Event *pre_other ();

   /// returns the pid of the process to which this event belongs
   unsigned pid () const;
   /// returns the process to which this event belongs
   Process *proc () const;
   /// returns a unique numeric id
   unsigned uid () const;
   std::string suid () const;
   /// returns a numeric id that is unique among events of the same process
   unsigned puid () const;

   inline unsigned depth_proc () const;
   inline unsigned depth_other () const;

   /// tests pointer equality of two events
   inline bool operator == (const Event &) const;
   /// returns a human-readable description of the event
   std::string str () const;

   /// true iff this event is the THSTART event of thread 0
   inline bool is_bottom () const;


   /// returns true iff (this <= e)
   inline bool is_predeq_of (const Event *e) const;
   /// returns true iff (this < e)
   inline bool is_pred_of (const Event *e) const;
   /// returns true iff (this # e)
   inline bool in_cfl_with (const Event *e) const;
   /// returns true iff (this \cup c is conflict-free)
   inline bool in_cfl_with (const Config &c) const;
   /// returns true iff (this \cup c is conflict-free); less efficient than previous
   inline bool in_cfl_with (const Cut &c) const;
   /// returns true iff (this and e are LOCK and siblings in the node[1] tree)
   inline bool in_icfl_with (const Event *e) const;

   /// returns some set of events in conflict which includes at least all
   /// immediate conflicts of "this"
   inline std::vector<Event*> icfls () const;

   /// the cut of the local configuration of the event
   const Primecon cone;

   /// depth of the event in the unfolding
   const unsigned depth;

private:
   inline const Event *pre_proc (bool bf) const;
   inline Event *pre_proc (bool bf);

   inline Eventbox *box_above () const;
   Eventbox *box_below () const;

   friend class EventIt;
};

// implementation of inline methods
#include "pes/event.hpp"

} // namespace dpu
#endif
