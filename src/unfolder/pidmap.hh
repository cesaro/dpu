
#ifndef _C15U_PIDMAP_HH_
#define _C15U_PIDMAP_HH_

#include <vector>

#include "pes/unfolding.hh"
#include "verbosity.h"

namespace dpu
{

class Pidmap
{
public:
   Pidmap () :
      map (1, 0),
      next_tid (1)
   {
      map.reserve (Unfolding::MAX_PROC);
      ASSERT (map.size() == 1);
      ASSERT (map.capacity() == Unfolding::MAX_PROC);
      ASSERT (map[0] == 0);
   }

   void add (unsigned k, unsigned v)
   {
      DEBUG ("c15u: pidmap: adding %u -> %u", k, v);
      ASSERT (k >= 1); // never main thread
      ASSERT (v >= 1); // never main thread

      // always +1 increments wrt to the upper limit.
      // Wrong, too strict when constructing replays
      //ASSERT (k <= map.size());
      ASSERT (k < map.capacity());

      if (k >= map.size()) map.resize (k + 1);
      map[k] = v;
   }

   unsigned get (unsigned k) const
      { ASSERT (k < map.size()); return map[k]; }

   void clear ()
      { next_tid = 1; map.resize (1); }

   unsigned size () const
      { return map.size(); }

   unsigned get_next_tid ()
      { return next_tid++; }

   void dump (bool steroids_to_dpu) const
   {
      unsigned i;

      PRINT ("== begin pidmap =="); 
      if (steroids_to_dpu)
         PRINT (" (replay -> dpu)");
      else
         PRINT (" (dpu -> replay)");

      ASSERT (map.size() >= 1);
      ASSERT (map[0] == 0);
      for (i = 0; i < map.size(); i++)
      {
         PRINT (" %s%u -> %s%u",
            steroids_to_dpu ? "r" : "#",
            i,
            steroids_to_dpu ? "#" : "r",
            map[i]);
      }
      PRINT ("== end pidmap =="); 
   }

protected:
   std::vector<unsigned> map;
   unsigned next_tid;
};

} // namespace

#endif

