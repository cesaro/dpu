#ifndef _C15U_DISSET_HH_
#define _C15U_DISSET_HH_

#include <vector>
#include <stdexcept>
#include "pes/event.hh"
#include "verbosity.h"

namespace dpu
{

class Disset
{
protected:
   /// a class to store internal information about every event stored in D
   class Elem {
   public:
      /// the event that we store in this element
      Event *e;
      /// the position (index) in the trail where this event had been inserted
      int idx;
      /// the position (index) of the trail where we have inserted an event that
      /// disables, aka "justifies", this event
      int disabler;

      /// next element in the list of justified or unjustified events
      Elem *next;
      /// previous element in the list of unjustified events
      Elem *prev;
   };

   class Ls;

   /// an iterator for the list of justified and unjustified events in D
   class It
   {
   public:
      bool operator== (const It &other) const { return ptr == other.ptr; }
      bool operator!= (const It &other) const { return ptr != other.ptr; }
      It &operator++ () { ptr = ptr->next; return *this; }
      It operator++ (int) { It r (*this); ptr = ptr->next; return r; }
      Event *operator* () { return ptr->e; }
   private:
      It (Elem *ptr_) : ptr (ptr_) {};
      Elem *ptr;
      friend class Ls;
   };

   /// a class that virtually represents a list of justified/unjustified events
   class Ls
   {
   public:
      It begin () const { return It (head); }
      It end () const { return It (nullptr); }
   private:
      Ls (Elem *&head_) : head (head_) {}
      Elem *&head;
      friend class Disset;
   };


   /// the set D in the C'15 algorithm, it behaves like a stack
   std::vector<Elem> stack;

   /// head of the singly-linked list of justified events in D; this list
   /// behaves like a stack, we use only the field "next" in every element
   Elem *just;

   /// head of the doubly-linked list of unjustified events in D; we use the
   /// fields "next" and "prev" of every element, as we need to support removals
   /// of elements in the middle of the list
   Elem *unjust;

   /// the disabler index of the top event in the list (stack) of justified
   /// events, or -1 if the list is empty
   int top_disabler;

   /// the index into the trail of the top element in the stack of events in D,
   /// or -1 if D is empty
   int top_idx;

   // operations to manage the stack of justified events
   inline bool just_isempty ();
   inline bool just_contains (Event *e) const;
   inline void just_push (Elem *e);
   inline Elem *just_pop ();
   inline Elem *just_peek ();

   // operations to manage the doubly-linked list of unjustified events
   inline bool unjust_contains (Event *e) const;
   inline void unjust_add (Elem *e);
   inline void unjust_remove (Elem *e);
   inline void unjust_remove_head ();
   inline bool unjust_isempty ();

public:
   /// constructor
   inline Disset ();

   /// add an event to D, given the event and the index it occupied the trail;
   /// the event will be removed from D whenever either we pop out of the trail
   /// the event at depth idx (this will happen in trail_pop) or when we call
   /// unadd, to revert the last addition to D
   inline void add (Event *e, int idx);

   /// undoes the last addition to D via add(e,idx)
   inline void unadd ();

   /// check if an event pushed to the trail justifies one in the disset,
   /// returns false iff e is in D (and so we are hitting a SSB)
   inline bool trail_push (Event *e, int idx);

   /// check if an event poped from the trail un-justifies one or more events
   /// that were, so far, justified, or removes one event from D
   inline void trail_pop (int idx);

   /// return true iff [e] intersects with the list of unjustified events in D
   inline bool intersects_with (const Event *e) const;

   /// return true iff the list of unjustified events in D intersects with the
   /// configuration denoted by the cut c
   inline bool intersects_with (const Cut &c) const;

   /// dumps debug information to stdout
   void dump () const;

   /// allows to get an InputIterator through the list of justified events in D
   const Ls justified = Ls (just);

   /// allows to get an InputIterator through the list of unjustified events in D
   const Ls unjustified = Ls (unjust);

   /// maximum number of events that a Disset can hold
   static const unsigned CAPACITY = 2048;

   /// number of sleep-set blocked executions encoutered so far
   unsigned ssb_count;
};

// implementation of inline methods
#include "unfolder/disset.hpp"

} //end of namespace
#endif
