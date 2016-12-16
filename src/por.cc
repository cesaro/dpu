///* Get a replay of a configuration */
#include "por.hh"

#include <vector>

namespace dpu
{

/*
 * check if all elements in combin are conflict-free in pairs
 */
bool is_conflict_free(std::vector<Event *> eset)
{
   for (unsigned i = 0; i < eset.size() - 1; i++)
     for (unsigned j = i; j < eset.size(); j++)
      {
        if (eset[i]->in_cfl_with(eset[j]))
           return false;
      }
   return true;
}

/// enumerate the comb for spike i. The temporatory combination is stored in temp.
/// If there exist a config satisfied, it will be assigned to J
void enumerate_combination (unsigned i, std::vector<std::vector<Event *>> comb , std::vector<Event*> temp, Config &J)
{
   DEBUG("This is enum_combination function");

   ASSERT(!comb.empty());

   for (unsigned j = 0; j < comb[i].size(); j++ )
   {
      if (j < comb[i].size())
      {
         temp.push_back(comb[i][j]);

         if (i == comb.size() - 1) // get a full combination
         {
            DEBUG_("temp = {");
            for (unsigned i = 0; i < temp.size(); i++)
               DEBUG_("%d, ", temp[i]->idx);

            DEBUG("}");



            /*
             * If temp is conflict-free, then J is assigned to union of every element's local configuration.
             */
            std::vector<Event *> lc;
            if (is_conflict_free(temp))
            {
               DEBUG(": a conflict-free combination");

               /// compute the local config for each event in temp and then add them to J

               for (unsigned i = 0; i < temp.size(); i++)
               {
                  lc = temp[i]->get_local_config();
                  for (int j = lc.size()-1; j >= 0; j--)
                     J.add(lc[j]); // BUG here, need to add event's local config
               }
               J.dump();
               return; // go back to find_alternative

               /*
               * Check if two or more events in J are the same: by updating cut of a config, don't need to check duplica any more
               */

            }
            else
               DEBUG(": not conflict-free");
         }
         else
            enumerate_combination(i+1, comb, temp, J);
      }

      temp.pop_back();
   }

}

bool find_alternative (Config &c, std::vector<Event*> d, Config &J)
{
      ASSERT (d.size ());

      J.clear();

      // huyen here

      std::vector<std::vector<Event *>> comb;

       DEBUG(" Find an alternative J to D after C: ");
       DEBUG_("   Original D = {");
         for (unsigned i = 0; i < d.size(); i++)
            DEBUG_("%d  ", d[i]->idx);
       DEBUG("}");

       for (auto ce : c.cex)
          ce->inside = 1;

       for (unsigned i = 0; i < d.size(); i++)
       {
          if (d[i]->inside == 1)
          {
             //remove D[i]
             d[i] = d.back();
             d.pop_back();
             i--;
          }
       }

       // set all inside back to 0
       for (auto ce : c.cex)
          ce->inside = 0;


       DEBUG_("   Prunned  D = {");
       for (unsigned i = 0; i < d.size(); i++) DEBUG_("%d  ", d[i]->idx);
       DEBUG("}");

       DEBUG_("   After C \n ");
       c.dump();
       /*
        *  D now contains only events which is in en(C).
        *  D is a comb whose each spike is a list of conflict events D[i].dicfl
        *  pour those in dicfl of all events in D into a comb whose each spike is corresponding to an event's dicfl
        */


       //DEBUG("D.size: %d", D.size());
       for (unsigned i = 0; i < d.size(); i++)
          comb.push_back(d[i]->dicfl);

       DEBUG("COMB: %d spikes: ", comb.size());
         for (unsigned i = 0; i < comb.size(); i++)
         {
            DEBUG_ ("  spike %d: (#e%d (len %d): ", i, d[i]->idx, comb[i].size());
            for (unsigned j = 0; j < comb[i].size(); j++)
               DEBUG_(" %d", comb[i][j]->idx);
            DEBUG("");
         }
       DEBUG("END COMB");


       /// remove from the comb those are also in D
       for (unsigned i = 0; i < comb.size(); i++)
       {
          DEBUG("   For comb[%d]:", i);

          unsigned j = 0;
          bool removed = false;

          DEBUG_("    Remove from comb[%d] those which are already in D: ", i);

          while ((comb[i].size() != 0) and (j < comb[i].size()) )
          {
             // check if there is any event in any spike already in D. If yes, remove it from spikes
             for (unsigned k = 0; k < d.size(); k++)
             {
                if (comb[i][j] == d[k])
                {
                   DEBUG(" %d ", comb[i][j]->idx);
                   //remove spike[i][j]
                   comb[i][j] = comb[i].back();
                   comb[i].pop_back();
                   removed = true;
                   DEBUG("Remove done");
                   break;
                }
             }

             // if we removed some event in spikes[i] by swapping it with the last one, then repeat the task with new spike[i][j]
             if (!removed)
                j++;
             else
                removed = false;

          }

          DEBUG("Done.");

          if (comb[i].size() == 0)
          {
             DEBUG("\n    comb %d (e%d) is empty: no alternative; returning.", i, d[i]->idx);
             return false;
          }


          /*
           *  Remove from spikes those are in conflict with any maxinal event
           *  Let's reconsider set of maximal events
           */
          removed = false;
          j = 0;
          Event * max;

          DEBUG("\n    Remove from comb[%d] events cfl with maximal events in C", i);

          while ((comb[i].size() != 0) and (j < comb[i].size()) )
          {
             for (int i = 0; i < c.num_procs(); i++)
             {
                DEBUG("LAN THU %d", i);
                max = c.proc_max(i); // get maximal event for proc i
                DEBUG("%p", max);

                if (max == nullptr) continue;

                if (comb[i][j]->in_cfl_with(max))
                {
                   //remove spike[i][j]
                   DEBUG_("    %d cfl with %d", comb[i][j]->idx, max->idx);
                   DEBUG("->Remove %d", comb[i][j]->idx);
                   comb[i][j] = comb[i].back();
                   comb[i].pop_back();
                   removed = true;
                   break;
                }
                else
                   DEBUG("Nothing");
             }

             DEBUG("Done.");
             // if we removed some event in spikes[i] by swapping it with the last one,
             // then repeat the task with new spike[i][j] which means "do not increase j"
             if (!removed)
                j++;
             else
                removed = false;
          }


          DEBUG("Done.");
          if (comb[i].size() == 0)
          {
             DEBUG("    Comb %d (e%d) is empty: no alternative; returning.", i, d[i]->idx);
             return false;
          }

       } // end for


       /// show the comb
       ASSERT (comb.size() == d.size ());
       DEBUG("COMB: %d spikes: ", comb.size());
       for (unsigned i = 0; i < comb.size(); i++)
       {
          DEBUG_ ("  spike %d: (#e%d (len %d): ", i, d[i]->idx, comb[i].size());
          for (unsigned j = 0; j < comb[i].size(); j++)
             DEBUG_(" %d", comb[i][j]->idx);
          DEBUG("");
       }
       DEBUG("END COMB");


       if (comb.size() == 0)
       {
          DEBUG("Empty comb");
          return false;
       }

       else
       {
          std::vector<Event *> temp;
          enumerate_combination(0, comb, temp, J);
          if (J.is_empty())
             return false;
          else
             return true;
       }

       return false;
}


} // end of namespace

