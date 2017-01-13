
#include "pes/process.hh"
#include "verbosity.h"

namespace dpu {
void Process::dump ()
{
   PRINT (" == process begin ==");
   PRINT (" this %p pid %u first-event %08x last-event %08x",
         this, pid(), first_event()->uid(), last->uid());

   ASSERT (first_event() and first_event()->action.type == ActionType::THSTART);
   for (Event &e : *this)
   {
      PRINT (" %s", e.str().c_str());
   }
   PRINT (" == process end ==");
}

} // namespace
