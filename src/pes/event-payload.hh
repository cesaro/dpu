
#ifndef __PES_EVENTPAYLOAD_HH_
#define __PES_EVENTPAYLOAD_HH_

#include <unistd.h>
#include <string>

namespace dpu
{

class EventPayload
{
public:

   virtual size_t pointed_memory_size () const
   {
      return 0;
   }

   virtual std::string str() const
   {
      return "";
   }
};

} // namespace dpu
#endif
