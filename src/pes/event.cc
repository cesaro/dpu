
#include "pes/event.hh"
#include "pes/eventbox.hh"
#include "pes/unfolding.hh"

namespace dpu {

Eventbox *Event::box_below () const
{
   return ((Eventbox *) this) - 1;
}

unsigned Event::pid () const
{
   return Unfolding::ptr2pid (this);
}

unsigned Event::uid () const
{
   return Unfolding::ptr2uid (this);
}

std::string Event::suid () const
{
   return fmt ("%0*x", 1 + int2msb (Unfolding::ALIGN) / 4, uid());
}

unsigned Event::puid () const
{
   return Unfolding::ptr2puid (this);
}

Process *Event::proc () const
{
   return Unfolding::ptr2proc (this);
}

#ifdef CONFIG_STATS_DETAILED
Event::Counters Event::counters;
#endif

} // namespace
