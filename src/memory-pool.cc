

#include "memory-pool.hh"

namespace dpu
{

void MemoryPool::assertt () const
{
   int i;

   if (regions.empty()) return;

   // check that each memory region is non-empty, disjoint with others, and
   // sorted by increasing order
   ASSERT (! regions[0].empty());
   for (i = 1; i < regions.size(); ++i)
   {
      ASSERT (! regions[i].empty());
      ASSERT (regions[i-1] < regions[i]);
      ASSERT (! regions[i-1].overlaps(regions[i]));
   }
}

} // namespace dpu
