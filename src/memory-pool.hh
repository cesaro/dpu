
#ifndef __MEMORYPOOL_HH_
#define __MEMORYPOOL_HH_

#include <vector>
#include <algorithm>

#include "pes/action.hh"
#include "memory-region.hh"

#include "verbosity.h"

namespace dpu
{

class MemoryPool
{
public :
   typedef std::vector<MemoryRegion<Addr>> Container;

   MemoryPool () :
      regions ()
   {}

   MemoryPool (Container &c) :
      regions (c)
   {}

   typedef Container::iterator iterator;
   typedef Container::const_iterator const_iterator;

   iterator begin () { return regions.begin(); }
   iterator end () { return regions.end(); }

   const_iterator begin () const { return regions.begin(); }
   const_iterator end () const { return regions.end(); }

   Addr lower () const { return regions.front().lower; }
   Addr upper () const { return regions.back().upper; }
   size_t span () const { return lower() - upper(); }

   /// Returns true iff memory pools *this and \p other have overlapping memory
   /// regions. When the answer is yes, \p inter will be a non-empty
   /// intersection of two overlapping regions.
   inline bool
      overlaps (const MemoryPool &other, MemoryRegion<Addr> &inter) const;

   size_t size () const { return regions.size(); }
   bool empty () const { return regions.empty(); }
   void clear () { return regions.clear(); }

   size_t pointed_memory_size () const
   {
      return sizeof(MemoryPool) + regions.size() * sizeof(MemoryRegion<Addr>);
   }

   void assertt () const;

   std::string str () const
   {
      std::string s;
      for (auto r : regions) s += r.str() + "\n";
      return s;
   }

private :
   Container regions;

   friend bool operator== (const MemoryPool &a, const MemoryPool &b);
};

inline bool operator== (const MemoryPool &a, const MemoryPool &b);
inline bool operator< (const MemoryPool &a, const MemoryPool &b);
inline bool operator> (const MemoryPool &a, const MemoryPool &b);

// implementation of inline methods
#include "memory-pool.hpp"

} // namespace dpu
#endif
