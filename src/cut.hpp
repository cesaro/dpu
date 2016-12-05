
#include <string.h>

/// creates an empty configuration
inline Cut::Cut (const Unfolding &u) :
   nrp (u.num_procs()),
   max (new Event* [nrp])
{
   int i;
   DEBUG ("Cut.ctor: this %p nrp %d", this, nrp);

   // initialize the size elements of the vector to null
   for (i = 0; i < nrp; i++)
      max[i] = nullptr;
}

/// creates a local configuration
inline Cut::Cut (const Unfolding &u, Event &e) :
   nrp (u.num_procs()),
   max (new Event* [nrp])
{
   int i;
   DEBUG ("Cut.ctor: this %p u.nump %d e %p", this, nrp, &e);

   // initialize the size elements of the vector to null
   for (i = 0; i < nrp; i++)
      max[i] = nullptr;

   // add the event e to its own process
   max[e.pid()] = &e;

   // now we should scan [e] to find the maximal events in each process, but I
   // don't have time to write this code
   ASSERT (0);
}

/// copy constructor
inline Cut::Cut (const Cut &other) :
   nrp (other.nrp),
   max (new Event* [nrp])
{
   DEBUG ("Cut.ctor: this %p other %p (copy)", this, &other);
   // copy the pointers
   memcpy (max, other.max, nrp * sizeof (Event*));
}

inline Cut::~Cut ()
{
   // delete the memory in the vector
   delete[] max;
}

inline void Cut::add (Event *e)
{
   DEBUG("Cut.add: this %p e %p e.pid %d", this, e, e->pid());
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
      ASSERT (e->pre_other()->vclock[e->pre_other()->pid()] <=
            max[e->pre_other()->pid()]->vclock[e->pre_other()->pid()]);
   }

   max[e->pid()] = e;
}

inline void Cut::clear ()
{
   unsigned i;
   for (i = 0; i < nrp; i++) max[i] = 0;
}

inline Event *Cut::operator[] (unsigned pid)
{
   ASSERT (pid < nrp);
   return max[pid];
}

inline unsigned Cut::num_procs()
{
   return nrp;
}

