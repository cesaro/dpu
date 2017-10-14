
#include "unfolder/disset.hh"


namespace dpu
{

void Disset::dump () const
{
   const Elem *e;

   PRINT ("== begin disset =="); 

   PRINT ("%zu events, top-idx %d, top-disabler %d, ssb-count %u",
         stack.size(), top_idx, top_disabler, ssb_count);
   PRINT ("Unjustified:");
   for (e = unjust; e; e = e->next)
   {
      PRINT  (" idx %d %s", e->idx, e->e->str().c_str());
   }

   PRINT ("Justified (top-down):");
   for (e = just; e; e = e->next)
   {
      PRINT (" idx %d dis %d %s", e->idx, e->disabler, e->e->str().c_str());
   }

   PRINT ("== end disset =="); 
}

} // namespace
