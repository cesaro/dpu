/* Get a replay of a configuration */
#include "por.hh"

#include <vector>

namespace dpu
{

void basic_conf_to_replay (Unfolding &u, BaseConfig &c, std::vector<int> &replay)
{
   DEBUG("Start computing replay");
   Event * pe;

   DEBUG("c.size=%d", c.size);
   for (unsigned i = 0; i < c.size; i++)
   {
      pe = c.max[i];
      pe->next = nullptr; //max[i].next = nullptr
      //DEBUG("%s type", action_type_str(pe->action.type));
      while (pe->action.type != ActionType::THSTART)
      {
         pe->color = 0;
         pe->pre_proc()->next = pe; // next event in the same process
         //pe->pre_other()->next  = pe;
         pe = pe->pre_proc();
      }
   }

//   for (unsigned i = 0; i < c.size; i++)
//   {
//      pe = c.max[i]->proc()->first_event();
//      while (pe)
//      {
//         DEBUG("%s type", action_type_str(pe->action.type));
//         pe = pe->next;
//      }
//   }

   bool unmarked = true; // there is some event in the configuration unmarked
   while (unmarked)
   {
      for (unsigned i = 0; i < c.size; i++)
      {
         pe = (c.max[i]->proc())->first_event();
//         DEBUG("%s type", action_type_str(pe->action.type));

         while (pe)
         {
            if (pe->color == 1)
               pe = pe->next;
            else
               if (((pe->pre_other() == nullptr) || (pe->pre_other()->color == 1)))
               {
//                  DEBUG("pre_other marked");
//                  DEBUG("pe->pid: %d", pe->pid());

                  if ((replay.empty()) || (replay.at(replay.size()-2) != pe->pid()))
                  {
                     replay.push_back(pe->pid());
                     replay.push_back(1);
                  }
                  else
                  {
                     replay.back()++;
                  }

                  pe->color = 1;
                  pe = pe->next;
               }
               else
                  break; // break while, then move to next process
         }
      }

      // terminate when all maximal events are marked.
      unsigned int j;
      for (j = 0; j < c.size; j++)
         if (c.max[j]->color == 0)
            break;

      if (j == c.size)
         unmarked = false;
   }

}



void LOCK_cex(Unfolding &u, Event *e)
{
   DEBUG("\n %p, id:%d: LOCK_cex", e);
   Event * ep, * em, *pr_mem;
   ep = e->pre_proc();
   em = e->pre_other();

   while (!(em->is_bottom()) and (em->vclock > ep->vclock)) //em is not a predecessor of ep
   {
      pr_mem   = em->pre_other()->pre_other(); // skip 2

      /*
       * Check if pr_mem < ep
       */
      if (ep->vclock > pr_mem->vclock)
         return;
      /*
       *  Need to check the event's history before adding it to the unf
       * Don't add events which are already in the unf
       */
     // Event * newevt = u.event(e->action, ep, pr_mem);

      // add new event newevt to set of dicfl of em and verse. Refer to the doc for more details
//      if (newevt->idx == u.back().idx)
//      {
//         add_to_dicfl(em, newevt);
//         add_to_dicfl(newevt, em);
//      }

      // move the pointer to the next
      //em = pr_mem;
      em = em->pre_other()->pre_other();
   }
}

void compute_cex(Unfolding & u, BaseConfig & c)
{
   Event *e;
   for (unsigned i = 0; i < c.size; i++)
   {
      e = c.max[i];
      while (e->action.type == ActionType::THSTART)
      {
         if (e->action.type == ActionType::MTXLOCK)
            LOCK_cex(u,e);
         e = e->pre_proc();
      }
   }
}

} // end of namespace
