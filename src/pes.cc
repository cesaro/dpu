/*
 * pes.cc
 *
 *  Created on: Jan 12, 2016
 *      Author: tnguyen
 */

#include <cstdint>
#include "pes.hh"

using std::vector;
using namespace ir;

namespace pes{

/*
 * function to compute a set of events enabled at a configuration c
 */
void Unfolding::compute_en(Config & c)
{
   Event e;
   ir::State & gs = *(c.gstate);
   std::vector<ir::Trans *> trans = gs.getSTrans();
   std::vector <ir::Process *> procs = gs.getSProc();
   for (auto it = procs.begin(); it != procs.end(); it++)
   {
	   std::vector<Trans *>::iterator i;
	   for (i = trans.begin(); i != trans.end(); i++)
	   {
		   if ( (**i).src == gs[(**it).id] )
		   {
			   e = new Event (*i);
			   evt.push_back(e);
			   Event * p = evt.back();
			   c.en.push_back(p);
		   }
	   }
   }
}

/*
 * function to compute a set of conflicting extension to a configuration
 */

void Unfolding::compute_cex(Config & c)
{

}

/*
 * function to add all extendable events to U
 */
void Unfolding:: extend(const Config & c)
{
   std::vector<Event *>::iterator it;
   for (it = c.en.begin(); it != c.en.end(); it++)
      U.push_back(*it);
   for (it = c.cex.begin(); it != c.cex.end(); it++)
	  U.push_back(*it);
}
/*
 * Compute set of events in conflict with event e
 */

void Unfolding::compute_cfl(Event & e)
{
   Event & parent = *(e.pre_mem);
   ir::Trans & trans = parent.getTrans();
   std::vector<Event *>::iterator it;
   switch (trans.type)
   {
      case ir::Trans::RD:
    	  // a read can access more than one global variable, for example l=x+y
    	  // divide into 2 different transition: l=x; l=l+y;
         for (it = parent.post_rdsyn.begin(); it != parent.post_rdsyn.end(); it++)
            if (*it != e)
        	   e.cfl.push_back(*it);
         break;
      case ir::Trans::WR: // only access one variable
    	 std::vector<std::vector<Event *>>::iterator i;
         for (i = parent.post_mem.begin(); i != parent.post_mem.end(); i++)
            if (find(i->begin(), i->end(), e) != i->end())
               for (it = i->begin(); it != i->end(); it++)
                  if (*i != e)
                     e.cfl.push_back(*it);
         break;
      case ir::Trans::SYN:
    	 for (it = parent.post_rdsyn.begin(); it != parent.post_rdsyn.end(); it++)
    	   if (*it != e)
    	      e.cfl.push_back(*it);
    	 break;
      case ir::Trans::LOC:
    	 Event & parent_loc = e.pre_proc;
    	 for (it = parent_loc.post_proc.begin(); it != parent_loc.post_proc.end(); it++)
    	    if (*it != e)
    	       e.cfl.push_back(*it);
    	 break;
   }
}

void Unfolding:: explore(Config & C, std::vector<Event *> D, std::vector<Event *> A)
{
   Event * e;
   extend(C);
   compute_en(C);
   if (C.en.empty() == true) return ;
   if (A.empty() == true)
       e = C.en.back();
   else
   {
      std::vector <Event *>::iterator it;
      for (it = A.begin(); it != A.end(); it++)
         if (std::find(C.en.begin(), C.en.end(), *it) != C.en.end()) //found an element of A in C
         {
            e = *it;
            A.erase(it);
         }
   }

   explore (C.add(*e), D, A);
}

/*
 * Add an event to a configuration
 */

Config & Config::add(Event & e)
{
   Config c = *this;
   ir::State & gs               = *(c.gstate);
   std::vector<Process *> procs = gs.getSProc();
   int numofproc                = procs.size();
   ir::Trans & s                = e.getTrans();
   ir::Process & p              = e.getProc();
   /*
    * update the event e
   */
   e.pre_proc = latest_proc[p.id];//e's parent is the latest event of the process
   e.post_proc.clear(); // e has no child

   switch (s.type)
   {
      case ir::Trans::RD:
         e.pre_mem  = latest_global_rdwr;
       	 e.post_mem[e.val] = e;
       	 break;

      case ir::Trans::WR:
         e.pre_mem  = latest_global_wr;
         latest_global_wr  = e;
      	 std::vector<Event *>::iterator it;
       	 for (it = e.post_mem.begin(); it < e.post_mem.end(); it++)
            it = e.val;
   	    // set pre-readers = set of latest events which use the variable copies of all processes.
   	    //size of pre-readers is numbers of copies of the variable
       	 for (it = e.pre_readers.begin(); it < e.pre_readers.end(); it++)
       	    for(int i = 0; i < numofproc; i++)
      		   *it = latest_global_rdwr[i][s.addr]; // the latest global event of the process
         break;
      case ir::Trans::SYN:
    	 break;
      case ir::Trans::LOC:
    	 break;
   }

   /*
    * update the configuration
    */

   c.latest_proc[s.proc] = &e; //update latest event of the process
   gstate = s.fire(gs); //update new global states

   //update local variables
   std::vector<int>::iterator i;
   for (i = s.localaddr.begin(); i != s.localaddr.end(); i++)
      c.latest_local_wr[*i] = &e;

   //update particular attribute according to type of transition.
   switch (s.type)
   {
      case ir::Trans::RD:
         c.latest_global_rdwr[s.proc][s.addr] = &e;
         break;

      case ir::Trans::WR:
    	    c.latest_global_wr[s.addr] = &e;
    	 for (i = 0; i < numofproc; i++)
       	    c.latest_global_rdwr[i][s.addr] = &e;

         break;

      case ir::Trans::SYN:
       	 c.latest_global_wr[s.addr]=&e;
       	 break;
   }

   return new Config(c);
}


void Unfolding::explore_rnd_config ()
{
   assert (U.size () == 0);

   // create an empty configuration conf;

   // in a loop:
      // choose randomly one event e enabled at conf;
      // conf.add (e); --> trigger construction of enabled events at the new
      //                   configuration, and addition of them to this->evt
      // reapeat until there is no event enabled

}

} // end of namespace

