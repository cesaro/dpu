
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
   fire (e);
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

void Cut::fire (Event *e)
{
   // a cut is the set of maximal events of some unique configuration; this
   // method adds to that configuration one of its enabled events, and updates
   // the cut so that it is the cut of the new resulting configuration

   DEBUG("Cut.fire: this %p nrp %d e %p e.pid %d", this, nrp, e, e->pid());
   ASSERT (e);
   ASSERT (e->pid() < nrp);

   // the unfolding might have changed the number of process after this
   // configuration was constructed; assert it didn't happen
   ASSERT (e->pid() < nrp);

   // adding e to the cut must not break causal-closendness
   ASSERT (e->pre_proc() == max[e->pid()]);
   ASSERT (!e->pre_other() or max[e->pre_other()->pid()]);
   ASSERT (!e->pre_other() or e->pre_other()->is_predeq_of (max[e->pre_other()->pid()]));

   // the new event cannot be in conflict with any other event in the cut
#ifdef CONFIG_DEBUG
   for (unsigned i = 0; i < nrp; i++)
      if (max[i])
         ASSERT (! e->in_cfl_with (max[i]));
#endif

   // this is all we have to do
   max[e->pid()] = e;
}

void Cut::unfire (Event *e)
{
   // a cut is the set of maximal events of some unique configuration; this
   // method removes from that configuration a maximal event and sets the cut to
   // be the cut of the resulting configuration; NOTE that the vector max[] may
   // contain events that are not maximal in the configuration (even if they are
   // the maximal event of some process), and as a result the test
   // max[e->pid()] == e is only a necessary, and not sufficient, condition to
   // remove e from the cut without breaking it

   // assertions
   ASSERT (e);
   ASSERT (max[e->pid()] == e);
   for (unsigned i = 0; i < nrp; i++)
      if (max[i])
         ASSERT (! e->is_pred_of (max[i]));

   // this is all we have to do
   max[e->pid()] = e->pre_proc();
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
      if (e) DEBUG("Proc %d", i);
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
