
#include <string.h>

#include "verbosity.h"

#include "pes/cut.hh"
#include "pes/event.hh"
#include "pes/unfolding.hh"

namespace dpu {

Cut::Cut (const Unfolding &u) :
   Cut (u.num_procs())
{
}

Cut::Cut (const Cut &other, Event *e) :
   Cut (std::max (other.nrp, e->pid() + 1))
{
   unsigned i;
   for (i = 0; i < other.nrp; i++) max[i] = other.max[i];
   for (; i < nrp; i++) max[i] = 0;
   add (e);
}


Cut::Cut (const Cut &c1, const Cut &c2) :
   nrp (c1.nrp > c2.nrp ? c1.nrp : c2.nrp),
   max (new Event* [nrp])
{
   unsigned i;
   DEBUG ("Cut.ctor: this %p c1 %p c2 %p nrp %d (max)", this, &c1, &c2, nrp);

   // optimization for the case when both cuts are the same (very often)
   if (&c1 == &c2)
   {
      memcpy (max, c1.max, nrp * sizeof (Event*));
      return;
   }

   // otherwise we compute the maximum per process
   for (i = 0; i < nrp; i++)
   {
      if (! c1[i])
      {
         max[i] = c2[i];
         continue;
      }
      if (! c2[i])
      {
         max[i] = c1[i];
         continue;
      }
      max[i] = c1[i]->depth_proc() > c2[i]->depth_proc() ? c1[i] : c2[i];
   }
}

void Cut::add (Event *e)
{
   DEBUG("Cut.add: this %p nrp %d e %p e.pid %d", this, nrp, e, e->pid());
   ASSERT (e);
   ASSERT (e->pid() < nrp);

   // the unfolding might have changed the number of process after this
   // configuration was constructed; assert it didn't happen
   ASSERT (e->pid() < nrp);

   // pre-proc must be the event max[e.pid()] // Why? Can we add an arbitray event to a Cut
   ASSERT (e->pre_proc() == max[e->pid()]);

   // similarly, pre_other needs to be a causal predecessor of the max in that
   // process; the following assertion is necessary but not sufficient to
   // guarantee it
   if (e->pre_other())
   {
      ASSERT (max[e->pre_other()->pid()]);
//      ASSERT (e->pre_other()->vclock[e->pre_other()->pid()] <=
//            max[e->pre_other()->pid()]->vclock[e->pre_other()->pid()]);
   }

   max[e->pid()] = e;
}

void Cut::dump () const
{
   DEBUG("== begin cut =="); 
   __dump_cut ();
   DEBUG("== end cut =="); 
}

void Cut::__dump_cut () const
{
   Event * e;
   unsigned i;

   for (i = 0; i < nrp; i++)
   {
      e = max[i] ;
      DEBUG("Proc %d, max %p", i, e);
      for (; e; e = e->pre_proc())
      {
         DEBUG (" %s", e->str().c_str());
      }
   }
}

std::string Cut::str () const
{
   std::string s;
   unsigned i;

   // [0: 0x123123; 1: 0x12321; others: 0]
   s = "[";
   for (i = 0; i < nrp; i++)
   {
      if (max[i]) s += fmt ("%u: %p; ", i, max[i]);
   }
   s += "other: 0]";
   return s;
}

} // namespace
