
#include "pes/unfolding.hh"
#include "pes/eventbox.hh"
#include "pes/event.hh"

namespace dpu {

Event *Eventbox::event_below () const
{
   return ((Event *) this) - 1;
}

unsigned Eventbox::pid () const
{
   return Unfolding::ptr2pid (this);
}

} // namespace
