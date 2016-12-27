
#include "pes/config.hh"

namespace dpu {

void Config::dump () const
{
   DEBUG("== begin config =="); 
   __dump_cut ();
   __dump_mutexes ();
   DEBUG("== end config =="); 
}

void Config::__dump_mutexes () const
{
   for (auto &pair : mutexmax)
   {
      DEBUG ("Addr %-#16lx e %p", pair.first, pair.second);
   }
}

} // namespace
