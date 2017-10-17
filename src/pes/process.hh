#ifndef __PES_PROCESS_HH_
#define __PES_PROCESS_HH_

#include "pes/event.hh"
#include "pes/unfolding.hh"

namespace dpu
{

class Process
{
protected :
   template <class T>
   class It
   {
   public :
      It (T *e) : e (e) {}

      bool operator== (const It &other) const
         { return e == other.e; }
      bool operator!= (const It &other) const
         { return e != other.e; }

      It &operator++ ()
         { e = e->flags.boxlast ? e->box_above()->event_above() : e + 1; return *this; }
      It operator++ (int x)
         { It copy = *this; ++(*this); return copy; }

      T &operator* () { return *e; }
      T *operator-> () { return e; }

   private:
      /// the event currently pointed by this iterator
      T *e;
   };

public :
   /// Constructor, \p creat is null for the first process or the first THCREAT
   /// event that created this process. \p u is the unfolding this process
   /// belongs to.
   inline Process (Event *creat, Unfolding *u);

   /// pretty print of the process' events
   void dump ();

   /// iterator over the events in this process
   inline It<Event> begin ();
   inline It<Event> end ();
   inline const It<const Event> begin () const;
   inline const It<const Event> end () const;

   /// Returns the pid of this process
   inline unsigned pid () const;

   /// Returns the offset of a pointer within the memory pool of this process
   inline size_t offset (void *ptr) const;

   /// Returns the unfolding this process belongs to
   Unfolding *unfolding () { return u; }
   const Unfolding *unfolding () const { return u; }

   /// returns the first event (necessarily a THSTART) of this process
   inline Event *first_event () const;

   /// THSTART, 0 process predecessor, 1 predecessor (THCREAT) in another process
   inline Event *add_event_0p (Event *creat);
   /// THCREAT(tid), THEXIT(), one predecessor (in the process)
   inline Event *add_event_1p (Action ac, Event *p);
   /// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
   inline Event *add_event_2p (Action ac, Event *p, Event *m);

   /// returns the number of bytes of memory consumed by this process
   size_t memory_size () const
      { return (size_t) (((char*) last) - (char*) this) + sizeof (Event); }

   /// returns the memory size of the data (indirectly) pointed by fields in this object
   inline size_t pointed_memory_size () const;

   struct {
      unsigned events;
   } counters;

private :
   /// Pointer to the unfolding this process belongs to
   Unfolding *u;

   /// Last event inserted in the memory pool of this process
   Event *last;

   /// returns a pointer to the first box (lowest address) in the process
   inline Eventbox *first_box () const;
   /// raises an exception iff we have no more space to store new events
   inline void have_room () const;
};

// implementation of inline methods
#include "pes/process.hpp"

} // namespace dpu
#endif
