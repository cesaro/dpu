
#include "c15u/trail.hh"

namespace dpu
{

void Trail::dump () const
{
   unsigned i;

   PRINT ("== begin trail =="); 
   for (i = 0; i < size(); i++)
   {
      PRINT ("i %2u %s", i, data()[i]->str().c_str());
   }
   PRINT ("== end trail =="); 
}

} // namespace
