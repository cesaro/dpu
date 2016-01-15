/*
 * pes.cc
 *
 *  Created on: Jan 12, 2016
 *      Author: tnguyen
 */

#include "pes.hh"

using std::vector;
using namespace ir;

namespace pes{

/*
 * function to compute a set of events enabled at the state of current configuration
 */
std::vector<Event *> Config::compute_en()
{
   std::vector<Event *> en;
   ir::State & gs=*gstate;
   std::vector<Process>::iterator it;
   Event * e;
   int count=0;
   for (it=gs.m.procs.begin(); it!=gs.m.procs.end(); it++)
   {
	   std::vector<Trans *>::iterator i;
	   for (i=(*it.))
	   for (i=(*it).trans.begin(); i!=(*it).trans.end(); i++)
	      if    ==gs[count])
	         e=new Event(*i);

   }



   return en;
}
/*
 * function to compute a set of conflicting extension to the current configuration
 */

std::vector<Event *>Config::compute_cex()
{
   std::vector<Event *> cex;
   return cex;
}
void Config::update(Event & e)
{
}

/*
 * function to add new event to a configuration and update necessary properties
 */
void Config::add(Event & e)
{
   ir::Trans * s=e.getTrans();
   ir::State & gs=*gstate;
   int numofproc=gs.m.procs.size();
   /*
    * update the event e
   */
   e.pre_proc=latest_proc[s->proc];//e's parent is the latest event of the process
   e.post_proc.clear(); // e has no children

   switch (s->type)
   {
      case ir::Trans::RD:
         e.pre_mem = latest_global_rdwr;
       	 e.post_mem[e.val]=e.val;

      case ir::Trans::WR:
         e.pre_mem=latest_global_wr;
         this->latest_global_wr->post_mem[]
      	 std::vector<Event *>::iterator it;
       	 for (it=e.post_mem.begin();it<e.post_mem.end();it++)
           it=e.val;
   	    // set pre-readers = set of latest events which use the variable in all processes.
   	    //size of pre-readers is numbers of copies of the variable
       	 for (it=e.pre_readers.begin();it<e.pre_readers.end();it++)
       	    for(i=0;i<numofproc;i++)
      		   it=latest_global_rdwr[i][s->addr]; // the latest global event of the process

      case ir::Trans::SYN:
      case ir::Trans::LOC:
   }

   /*
    * update the configuration
    */

   this->latest_proc[s->proc]=&e; //update latest event of the process
   gstate=s->fire(gs); //update new global states

   //update local variables
   std::vector<uint32_t>::iterator i;
   for (i=s->localaddr.begin();i!=s->localaddr.end();i++)
      this->latest_local_wr[*i]=&e;

   //update particular attribute according to type of transition.
   switch (s->type)
      case ir::Trans::RD:
         this->latest_global_rdwr[s->proc][s->addr] = &e;
         break;

      case ir::Trans::WR:
    	 this->latest_global_wr[s->addr] = &e;
    	 for (i=0; i<numofproc; i++)
       	    this->latest_global_rdwr[i][s->addr] = &e;

         break;

      case ir::Trans::SYN:
       	 this->latest_global_wr[s->addr]=&e;
       	 break;


}

} // end of namespace






