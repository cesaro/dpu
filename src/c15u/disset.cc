
#include "c15u/disset.hh"


namespace dpu
{

void Disset::dump () const
{
   const Elem *e;

   DEBUG("== begin disset =="); 

   DEBUG ("%u events, top-idx %d, top-disabler %d, ssb-count %u",
         stack.size(), top_idx, top_disabler, ssb_count);
   DEBUG ("Unjustified:");
   for (e = unjust; e; e = e->next)
   {
      DEBUG (" idx %d %s", e->idx, e->e->str().c_str());
   }

   DEBUG ("Justified (top-down):");
   for (e = just; e; e = e->next)
   {
      DEBUG (" idx %d dis %d %s", e->idx, e->disabler, e->e->str().c_str());
   }

   DEBUG("== end disset =="); 
}

} // namespace
