
// class Primecon and inline methods needs to be included exactly in the order
// event.hh includes them to avoid circular inclusions
#include "pes/event.hh"

#ifndef __PES_PRIMECON_HH_
#define __PES_PRIMECON_HH_

#include <vector>
#include "pes/cut.hh"
#include "pes/action.hh"

namespace dpu
{

class Config;

class Primecon : public Cut
{
public:
   /// start event in any process
   Primecon (unsigned n, Event *e);
   /// one predecessor
   Primecon (const Primecon &p, Event *e);
   /// two predecessors
   Primecon (const Primecon &p, const Primecon &m, Event *e);

   /// prints the cut in stdout
   void dump () const;

   /// returns a human-readable description
   std::string str () const;

   /// returns true iff (this # other)
   bool in_cfl_with (const Primecon *other) const;

   /// returns false iff (this \cup c is conflict-free)
   bool in_cfl_with (const Config &c) const;

   /// returns the maximal lock/unlock for address a
   const Event *mutex_max (Addr a) const;
   Event *mutex_max (Addr a);

   /// returns the memory size of the data pointed by fields in this object
   inline size_t pointed_memory_size () const;

protected :
   /// the list of maximal lock/unlock events sorted by the increasing value of
   /// the address
   std::vector<const Event*> lockmax;

   void __dump_lockmax () const;
};

} // namespace dpu
#endif
