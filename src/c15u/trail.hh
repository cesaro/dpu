
#ifndef _C15U_TRAIL_HH_
#define _C15U_TRAIL_HH_

#include <vector>
#include "pes/event.hh"
#include "verbosity.h"

namespace dpu
{

class Trail : public std::vector<Event*>
{
public:

   Trail () :
      nrctxsw (0),
      lastpid (0)
      {}

   void push (Event *e)
   {
      ASSERT (e);
      _update_nrctxsw_push (e->pid());
      std::vector<Event*>::push_back (e);
   }

   Event *pop ()
   {
      Event *e = back();
      std::vector<Event*>::pop_back();
      _update_nrctxsw_pop ();
      return e;
   }

   Event *peek ()
      { return back (); }

   void dump () const;
   void dump2 (const char *prefix) const;
   unsigned nr_context_switches () const
   {
      return nrctxsw;
   }

private:
   unsigned nrctxsw;
   unsigned lastpid;

   void _update_nrctxsw_push (unsigned newpid)
   {
      ASSERT (empty() or back()->pid() == lastpid);
      if (lastpid == newpid) return;
      nrctxsw++;
      lastpid = newpid;
   }

   void _update_nrctxsw_pop ()
   {
      unsigned newlastpid;

      newlastpid = size() ? back()->pid() : 0;
      if (newlastpid != lastpid) nrctxsw--;
      lastpid = newlastpid;
   }
};

} // namespace

#endif
