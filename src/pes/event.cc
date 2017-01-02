
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

Process *Event::proc () const
{
   return Unfolding::ptr2proc (this);
}

std::string Event::str () const
{
   std::string s;

   s = fmt ("e %-16p p %2d d %02u,%02u,%02u pre %-16p %-16p "
         "%c%c%c%c |rb| %lu ac %s",
         this, pid(), depth, node[0].depth, node[1].depth,
         pre_proc(), pre_other(),
         flags.boxfirst ? 'f' : '-',
         flags.boxlast ? 'l' : '-',
         flags.crb ? 'c' : '-',
         flags.ind ? 'D' : '-',
         redbox.size (),
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

} // namespace
