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
std::vector<Event *> Unfolding::compute_en(const Config & c)
{
   std::vector<Event *> en;
   Event e;
   ir::State & gs = *(c.gstate);
   std::vector<ir::Trans *> trans = gs.getSTrans();
   std::vector <ir::Process *> procs = gs.getSProcs();
  // int * t = gs.getTab();
   std::vector <Process *>::iterator it;
   for (it = procs.begin(); it != procs.end(); it++)
   {
	   std::vector<Trans *>::iterator i;
	   for (i = trans.begin(); i != trans.end(); i++)
	   {
		   if ( (**i).src == gs[(**it).id] )
		   {
			   e = new Event (*i);
			   evt.push_back(e);
			   en.push_back(&evt.back());
		   }
	   }
   }
   return en;
}

/*
 * function to compute a set of conflicting extension to a configuration
 */

std::vector<Event *> Unfolding::compute_cex(const Config & c)
{
   std::vector<Event *> cex;

   return cex;
}

/*
 * function to add all extendable events to U
 */
void Unfolding:: extend(const Config & c)
{
   std::vector<Event *>::iterator it;
   for (it = compute_en(c).begin(); it != compute_en(c).end(); it++)
      U.push_back(*it);
   for (it = compute_cex(c).begin(); it != compute_cex(c).end(); it++)
	  U.push_back(*it);
}

void Unfolding:: explore(Config & C,std::vector<Event *> D, std::vector<Event *> A)
{
   Event * e;
   extend(C);
   std::vector<Event *> en = compute_en(C);
   if (en.empty() == true) return ;
   if (A.empty() == true)
       e = en.back();
   else
   {
      std::vector <Event *>::iterator it;
      for (it = A.begin(); it != A.end(); it++)
         if (std::find(en.begin(), en.end(), *it) == true)
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
   ir::Trans & s                = e.getTrans();
   ir::Process & p              = e.getProc();
   ir::State & gs               = *(c.gstate);
   std::vector<Process *> procs = gs.getSProcs();
   int numofproc                = procs.size();
   /*
    * update the event e
   */
   e.pre_proc = latest_proc[p.id];//e's parent is the latest event of the process
   e.post_proc.clear(); // e has no children

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

      case ir::Trans::SYN:
      case ir::Trans::LOC:
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

   return new Config(c);
}



} // end of namespace






