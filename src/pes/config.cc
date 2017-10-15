
#include "pes/config.hh"

namespace dpu {

void Config::__dump_mutexes () const
{
   for (auto &pair : mutexmax)
   {
      PRINT ("Addr %-#16lx e %08x", pair.first, pair.second->uid());
   }
}

} // namespace
