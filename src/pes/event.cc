
#include "pes/event.hh"
#include "pes/eventbox.hh"
#include "pes/process.hh"
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

const Process *Event::process () const
{
   return Unfolding::ptr2proc (this);
}

Process *Event::process ()
{
   return Unfolding::ptr2proc (this);
}

const Unfolding *Event::unfolding () const
{
   return process()->unfolding();
}

Unfolding *Event::unfolding ()
{
   return process()->unfolding();
}


std::string Event::str () const
{
   std::string s;

   // a nice event id is
   // fmt ("e%x%-6x", pid(), puid())

   s = fmt ("e %0*x p %2d d %02u,%02u,%02u pre %08x %08x "
         "%c%c%c%c dat %s ac %s",
         1 + int2msb (Unfolding::ALIGN) / 4,
         uid(), pid(), depth, node[0].depth, node[1].depth,
         pre_proc()->uid(), pre_other()->uid(),
         flags.boxfirst ? 'f' : '-',
         flags.boxlast ? 'l' : '-',
         flags.crb ? 'c' : '-',
         flags.ind ? 'D' : '-',
         dat ? dat->str().c_str() : "null",
         action_type_str (action.type));

   switch (action.type)
   {
   case ActionType::THCREAT :
      s += fmt (" val %d", action.val);
      break;
   case ActionType::THJOIN :
      s += fmt (" val %d", action.val);
      break;
   case ActionType::MTXLOCK :
      s += fmt (" addr %p", (void*) action.addr);
      break;
   case ActionType::MTXUNLK :
      s += fmt (" addr %p", (void*) action.addr);
      break;
   default :
      break;
   }
   return s;
}

#ifdef CONFIG_STATS_DETAILED
Event::Counters Event::counters;
#endif

} // namespace
