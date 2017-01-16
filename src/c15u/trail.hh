
#ifndef _C15U_TRAIL_HH_
#define _C15U_TRAIL_HH_

#include <vector>
#include "pes/event.hh"

namespace dpu
{

class Trail : public std::vector<Event*>
{
public:
   void push (Event *e)
      { std::vector<Event*>::push_back (e); }

   Event *pop ()
   {
      Event *e = back();
      std::vector<Event*>::pop_back();
      return e;
   }

   Event *peek ()
      { return back (); }

   void dump () const;
   void dump2 (const char *prefix) const;
};

} // namespace

#endif
