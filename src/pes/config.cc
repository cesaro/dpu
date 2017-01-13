
#include "pes/config.hh"

namespace dpu {

void Config::dump () const
{
   PRINT ("== begin config =="); 
   __dump_cut ();
   __dump_mutexes ();
   PRINT ("== end config =="); 
}

void Config::__dump_mutexes () const
{
   for (auto &pair : mutexmax)
   {
      PRINT ("Addr %-#16lx e %08x", pair.first, pair.second->uid());
   }
}

} // namespace
