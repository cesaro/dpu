
#include <string.h>

inline Cut::Cut (const Unfolding &u) :
   Cut (u.num_procs())
{
}

inline Cut::Cut (unsigned n) :
   nrp (n),
   max (new Event* [nrp])
{
   int i;
   DEBUG ("Cut.ctor: this %p nrp %d", this, nrp);
   ASSERT (nrp >= 1);

   // initialize the size elements of the vector to null
   for (i = 0; i < nrp; i++)
      max[i] = nullptr;
}

/// copy constructor
inline Cut::Cut (const Cut &other) :
   nrp (other.nrp),
   max (new Event* [nrp])
{
   DEBUG ("Cut.ctor: this %p other %p nrp %d (copy)", this, &other, nrp);
   // copy the pointers
   memcpy (max, other.max, nrp * sizeof (Event*));
}

inline Cut::Cut (const Cut &c1, const Cut &c2) :
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

inline Cut::Cut (unsigned n, Event *e) :
   Cut (n)
{
   add (e);
}

inline Cut::Cut (const Cut &other, Event *e) :
   Cut (std::max (other.nrp, e->pid() + 1))
{
   unsigned i;
   for (i = 0; i < other.nrp; i++) max[i] = other.max[i];
   for (; i < nrp; i++) max[i] = 0;
   add (e);
}

inline Cut::Cut (const Cut &c1, const Cut &c2, Event *e) :
   Cut (c1, c2)
{
   add (e);
}

inline Cut::~Cut ()
{
   // delete the memory in the vector
   DEBUG ("Cut.dtor: this %p nrp %d", this, nrp);
   delete[] max;
}

inline Cut & Cut::operator= (const Cut & other)
{
   DEBUG("Cut.op= this %p other %p", this, &other);
   nrp = other.nrp;
   max = new Event* [nrp];
   memcpy (max, other.max, nrp * sizeof (Event*));
   return *this;
}

inline Cut & Cut::operator= (Cut && other)
{
   DEBUG("Cut.op= this %p other %p (move)", this, &other);
   nrp = other.nrp;
   max = other.max;
   other.max = 0; // only suitable for destruction
   return *this;
}

inline void Cut::add (Event *e)
{
   DEBUG("Cut.add: this %p nrp %d e %p e.pid %d", this, nrp, e, e->pid());
   ASSERT (e);
   ASSERT (e->pid() < nrp);

   // the unfolding might have changed the number of process after this
   // configuration was constructed; assert it didn't happen
   ASSERT (e->pid() < nrp);

   // pre-proc must be the event max[e.pid()]
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

inline void Cut::clear ()
{
   unsigned i;
   for (i = 0; i < nrp; i++) max[i] = 0;
}

inline Event *Cut::operator[] (unsigned pid) const
{
   return pid < nrp ? max[pid] : nullptr;
}

inline Event *&Cut::operator[] (unsigned pid)
{
   ASSERT (pid < nrp);
   return max[pid];
}

inline unsigned Cut::num_procs() const
{
   return nrp;
}

