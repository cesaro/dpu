
// class Cut and inline methods needs to be included exactly in the order
// event.hh includes them to avoid circular inclusions
#include "pes/event.hh"

#ifndef __PES_CUT_HH_
#define __PES_CUT_HH_

// needed in the .hpp
#include <string>
#include <cstring>
#include "verbosity.h"

namespace dpu
{

class Event;
class Unfolding;

class Cut
{
public:
   /// creates an empty cut for as much as u.num_procs processes
   Cut (const Unfolding &u);

   /// creates an empty cut for as much as n processes
   inline Cut (unsigned n);

   /// copy constructor
   inline Cut (const Cut &other);

   /// max of two cuts
   Cut (const Cut &c1, const Cut &c2);

   /// destructor
   virtual inline ~Cut ();

   /// assignment operator
   inline Cut & operator= (const Cut & other);

   /// move-assignment operator
   inline Cut & operator= (Cut && other);

   /// fires an event enabled at the cut and adds it to the cut
   void fire (Event *e);

   /// removes an event of the cut and updates the cut with its predecessors
   void unfire (Event *e);

   /// updates the Cut so that it becomes the cut of the configuration
   /// [this] \cup [e]
   void unionn (Event *e);

   /// empties the configuration
   inline void clear ();

   /// prints the cut in stdout
   void dump () const;

   /// returns a human-readable description
   std::string str () const;

   /// returns true if the cut (configuration) is empty
   inline bool empty() const;

   /// returns true iff event e \in en(C), where C is this Cut
   inline bool enabled (const Event *e) const;

   /// returns true iff event e \in ex(C), where C is this Cut
   inline bool extension (const Event *e) const;

   /// returns true iff event e \in cex(C), where C is this Cut
   inline bool cex (const Event *e) const;

   /// assumes that e is in ex(C); returns true iff e in cex(C)
   bool ex_is_cex (const Event *e) const;

   /// returns the maximal event of process pid in the cut
   inline Event *operator[] (unsigned pid) const;
   inline Event *&operator[] (unsigned pid);

   /// returns the number of processes of the unfolding
   inline unsigned num_procs () const;

   /// sets the Event::color field for all events in the Cut
   void colorize (unsigned color);

   /// returns the memory size of the data pointed by fields in this object
   inline size_t pointed_memory_size () const;

protected:
   /// size of the map below (u.num_procs())
   unsigned nrp;

   /// map from process id (int) to maximal event in that process
   Event **max;

   void __dump_cut () const;

   // class Primecon uses the following three constructors
   inline Cut (unsigned n, Event *e);
   Cut (const Cut &other, Event *e);
   inline Cut (const Cut &c1, const Cut &c2, Event *e);
};

} // namespace dpu
#endif
