
#include "c15u/trail.hh"

namespace dpu
{

void Trail::dump () const
{
   unsigned i;

   DEBUG("== begin trail =="); 
   for (i = 0; i < size(); i++)
   {
      DEBUG ("i %2u %s", i, data()[i]->str().c_str());
   }
   DEBUG("== end trail =="); 
}

} // namespace
