
#ifndef _C15U_PIDPOOL_HH_
#define _C15U_PIDPOOL_HH_

#include <unordered_set>
#include <vector>

#include "pes/unfolding.hh"
#include "pes/event.hh"

namespace dpu
{

class Pidpool
{
public:
   inline Pidpool (Unfolding &u);

   inline unsigned create (const Event *c);
   inline void join (const Event *e);
   inline void clear ();
   void dump () const;

protected:
   struct Prochist
   {
      std::vector<int> pids;
      unsigned currdepth;
      std::unordered_set<unsigned> assigned;
      std::unordered_set<unsigned> joined;
   };

   Unfolding &u;
   std::vector<Prochist> procs;

   inline Prochist * new_prochist ();
   inline void update_hist_create (Prochist *h, unsigned pid);
};

// implementation of inline methods
#include "pidpool.hpp"

} // namespace

#endif
