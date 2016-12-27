#ifndef __PES_EVENTBOX_HH_
#define __PES_EVENTBOX_HH_

// needed for the .hpp
#include "verbosity.h"

namespace dpu
{

class Event;

class Eventbox
{
public:
   /// constructor
   inline Eventbox (Event *pre);
   /// first event contained in the box, undefined if the box contains no event
   inline Event *event_above () const;
   /// the last event of the box below, undefined if this is the first box
   inline Event *event_below () const;
   /// process causal predecessor of all events in the box
   inline Event *pre () const;
   /// returns the id of the process containing this event box
   unsigned pid () const;

private:
   /// the event that causally precedes all events in this box
   Event *_pre;
};

// implementation of inline methods
#include "pes/eventbox.hpp"

} // namespace dpu
#endif
