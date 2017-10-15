

#include "memory-pool.hh"

namespace dpu
{

void MemoryPool::assertt () const
{
   int i;

   // each redbox stores a non-empty range of memory addresses
   ASSERT (begin() != end());
   ASSERT (! empty ());
   ASSERT (! regions[0].empty());

   // check that the memory regions are non-empty, disjoint, and sorted by
   // increasing order
   for (i = 1; i < regions.size(); ++i)
   {
      ASSERT (! regions[i].empty());
      ASSERT (regions[i-1] < regions[i]);
      ASSERT (! regions[i-1].overlaps(regions[i]));
   }
}

} // namespace dpu
