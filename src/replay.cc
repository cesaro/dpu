/* Get a replay of a configuration */
#include <vector>
#include "replay.hh"

namespace dpu
{

void basic_conf_to_replay (Unfolding &u, BaseConfig &c, std::vector<int> &replay)
{
   Event * pe;
   unsigned int size = sizeof(c.max)/ sizeof(Event *);
   for (unsigned i = 0; i < size; i++)
   {
      pe = c.max[i];
      while (pe->action.type != ActionType::THSTART)
      {
         pe->color = 0;
         pe->pre_proc()->next = pe; // next event in the same process
         //pe->pre_other()->next  = pe;
      }
   }

   bool unmarked = true; // there is some event in the configuration unmarked
   while (unmarked)
   {
      for (unsigned i = 0; i < size; i++)
      {
         pe = c.max[i]->proc()->first_event();
         while (pe != c.max[i])
         {
            if ((pe->color == 0) && ((pe->pre_other() == nullptr) || (pe->pre_other()->color == 1) ) )
            {
               if (replay[replay.size() - 2] == pe->pid())
                  replay.back()++;
               else
               {
                  replay.push_back(pe->pid());
                  replay.push_back(1);
               }
               pe->color = 1;
               pe = pe->next;
            }
            else
               if (pe->color == 1)
                  pe = pe->next;
               else
                  break; // move to next process
         }
      }

      // terminate when all maximal events are marked.
      unsigned int j;
      for (j = 0; j < size; j++)
         if (c.max[j]->color == 0)
            break;

      if (j == size)
         unmarked = false;
   }

}
} // end of namespace
