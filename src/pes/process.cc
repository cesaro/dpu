
#include "pes/process.hh"

namespace dpu {
void Process::dump ()
{
   printf (" == process begin ==\n");
   printf (" this %p pid %u first-event %p last-event %p\n",
         this, pid(), first_event(), last);

   ASSERT (first_event() and first_event()->action.type == ActionType::THSTART);
   for (Event &e : *this)
   {
      DEBUG (" %s", e.str().c_str());
   }
   printf (" == process end ==\n");
}

} // namespace
