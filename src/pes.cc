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
   std::vector <Process *>::iterator it;
   for (it = procs.begin(); it != procs.end(); it++)
   {
	   std::vector<Trans *>::iterator i;
	   for (i = trans.begin(); i != trans.end(); i++)
	   {
		   if ( (**i).src == gs[(**it).id] )
		   {
			   e = new Event (*i);
               e.update(c);
			   evt.push_back(e);
			   Event * p = evt.back();
			   c.en.push_back(p);
			   U.push_back(p);
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
 * Compute set of events in conflict with event e
 */

void Event::compute_cfl(Event & e)
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
   compute_en(C);
   compute_cex(C);
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
   C.add(*e);
   explore (C, D, A);
}

/*
 * Add an event to a configuration
 */

void Config::add(Event & e)
{
   ir::State & gs               = *gstate;
   std::vector<Process *> procs = gs.getSProc();
   int numofproc                = procs.size();
   ir::Trans & s                = e.getTrans();
   ir::Process & p              = e.getProc();
   /*
    * update the configuration
    */

   latest_proc[s.proc] = &e; //update latest event of the process
   gstate = s.fire(gs); //update new global states
   latest_global_wr  = e;

   //update local variables in trans
   std::vector<int>::iterator i;
   for (i = s.localaddr.begin(); i != s.localaddr.end(); i++)
      latest_local_wr[*i] = &e;

   //update particular attributes according to type of transition.
   switch (s.type)
   {
      case ir::Trans::RD:
         latest_global_rdwr[s.proc][s.addr] = e;
         break;

      case ir::Trans::WR:
    	 latest_global_wr[s.addr] = e;
    	 for (i = 0; i < numofproc; i++)
       	    latest_global_rdwr[i][s.addr] = e;

         break;

      case ir::Trans::SYN:
       	 latest_global_wr[s.addr]=&e;
       	 break;
   }

}

void Event::update(Config & c)
{
  ir::Trans & s = getTrans();
  ir::Process & p = getProc();
  std::vector<Process *> procs = c.gstate->getSProc();
  pre_proc = c.latest_proc[p.id];//e's parent is the latest event of the process
  post_proc.clear(); // e has no child

  switch (s.type)
  {
     case ir::Trans::RD:
        pre_mem  = c.latest_global_rdwr;
      	post_mem[val] = *this;
      	 break;

     case ir::Trans::WR:
        pre_mem  = c.latest_global_wr;
        for (auto it = post_mem.begin(); it < post_mem.end(); it++)
           *it = *val;
  	    // set pre-readers = set of latest events which use the variable copies of all processes.
  	    //size of pre-readers is numbers of copies of the variable
      	 for (auto it = pre_readers.begin(); it < pre_readers.end(); it++)
      	    for(int i = 0; i < procs.size(); i++)
     		   *it = c.latest_global_rdwr[i][s.addr]; // the latest global event of the process
        break;
     case ir::Trans::SYN:
   	 break;
     case ir::Trans::LOC:
   	 break;
  }
}

} // end of namespace






