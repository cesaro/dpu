
#include "unfolder/trail.hh"

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


void Trail::dump2 (const char *prefix) const
{
   // #0 s c1;  #1 s x00;  #0 x00;  #1 x00 e;  #0 x00 j1 e

   unsigned pid, i, j;
   Event * const * tab;
   Event *e;
   std::vector<Addr> addrs;

   tab = data();
   pid = 0;
   ASSERT (size() == 0 or tab[0]->pid() == pid);
   PRINT_ ("%s#0", prefix);
   for (i = 0; i < size(); i++)
   {
      e = tab[i];
      if (e->pid() != pid)
      {
         pid = e->pid();
         PRINT_ (";  #%u", pid);
      }
      if (i % 5 == 0) PRINT_ (" @%u", i);
      switch (e->action.type)
      {
      case ActionType::THCREAT :
         PRINT_ (" C%lu", e->action.val);
         break;
      case ActionType::THJOIN :
         PRINT_ (" J%lu", e->action.val);
         break;
      case ActionType::THSTART :
         PRINT_ (" S");
         break;
      case ActionType::THEXIT :
         PRINT_ (" E");
         break;
      case ActionType::MTXLOCK :
         // find the addr
         for (j = 0; j < addrs.size(); j++)
            if (addrs[j] == e->action.addr) break;
         if (j == addrs.size())
            addrs.push_back (e->action.addr);
         // print it
         if (i < size() - 1 and
               tab[i+1]->action.type == ActionType::MTXUNLK and
               tab[i+1]->action.addr == e->action.addr)
         {
            i++;
            PRINT_ (" X%02u", j);
            if (i % 5 == 0) PRINT_ (" @%u", i + 1);
         }
         else
            PRINT_ (" L%02u", j);
         break;
      case ActionType::MTXUNLK :
         // find the addr
         for (j = 0; j < addrs.size(); j++)
            if (addrs[j] == e->action.addr) break;
         if (j == addrs.size())
            addrs.push_back (e->action.addr);
         PRINT_ (" U%02u", j);
         break;
      default :
         ASSERT (0);
      }
   }
   PRINT ("");
}

} // namespace
