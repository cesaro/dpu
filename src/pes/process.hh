#ifndef __PES_PROCESS_HH_
#define __PES_PROCESS_HH_

#include "pes/event.hh"
#include "pes/unfolding.hh"

namespace dpu
{

class EventIt
{
public :
   bool operator== (const EventIt &other) const
      { return e == other.e; }
   bool operator!= (const EventIt &other) const
      { return e != other.e; }

   EventIt &operator++ ()
      { e = e->flags.boxlast ? e->box_above()->event_above() : e + 1; return *this; }
   EventIt operator++ (int x)
      { EventIt copy = *this; ++(*this); return copy; }

   Event &operator* ()
      { return *e; }
   Event *operator-> ()
      { return e; }

private:
   /// the event currently pointed by this iterator
   Event *e;

   /// constructor
   EventIt (Event *e) :
      e (e)
      { }
   friend class Process;
};

class Process
{
public :
   /// ctor, creat is null for the first process or the THCREAT event that
   /// created this process
   inline Process (Event *creat);

   /// pretty print of the process' events
   void dump ();

   /// iterator over the events in this process
   inline EventIt begin ();
   inline EventIt end ();

   /// returns the pid of this process
   inline unsigned pid () const;
   /// returns the offset of a pointer within the memory pool of this process
   inline size_t offset (void *ptr) const;

   /// returns the THSTART event of this process
   inline Event *first_event () const;

   /// THCREAT(tid), THEXIT(), one predecessor (in the process)
   inline Event *add_event_1p (Action ac, Event *p);
   /// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
   inline Event *add_event_2p (Action ac, Event *p, Event *m);

   struct {
      long unsigned events;
   } counters;

private :
   /// last event inserted in the memory pool of this process
   Event *last;

   inline Eventbox *first_box () const;
};

// implementation of inline methods
#include "pes/process.hpp"

} // namespace dpu
#endif
