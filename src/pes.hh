
#ifndef __PES_HH_
#define __PES_HH_

#include <ir.hh>

namespace pes
{

class Event
{
public:
   Event *              pre_proc;    // for all events
   std::vector<Event *> post_proc;

   Event *              pre_mem;     // for all events except LOCAL
   std::vector<Event *> post_mem;

   std::vector<Event *> pre_readers; // only for WR events

   uint32_t             val;
   std::vector<uint32_t>localvals;
   Trans *              trans;
}

} // namespace pes

#endif 
