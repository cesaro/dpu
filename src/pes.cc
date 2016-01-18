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
   Event * e;
   ir::State & gs=*gstate;
   int numprocs=gs.m.getNumofProcs();
   std::vector<Trans *> trans=gs.m.getTrans();

   uint32_t & t = gs.getTab();
   for (int i=0; i<numprocs; i++)
   {

      std::vector<Trans *>::iterator it;
	  for (it=trans.begin(; it!=trans.end(); it++)
         if ()




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

   //update the configuration
   //update new global states
   gstate=s->fire(gs);
   latest_proc[s->proc]=&e;
   //update local variables
   std::vector<uint32_t>::iterator i;
   for (i=s->localaddr.begin();i!=s->localaddr.end();i++)
      this->latest_local_wr[*i]=&e;

   switch (s->type)
      case ir::Trans::RD:
      {
         this->latest_global_rdwr[s->proc][s->addr]=&e;
         break;
      }
      case ir::Trans::WR:
      {
       	 this->latest_global_wr[s->addr]=&e;
       	 this->latest_global_rdwr[s->proc][s->addr]=&e;
         break;
      }
      case ir::Trans::SYN:
      {
       	 this->latest_global_wr[s->addr]=&e;
       	 break;
      }
   //update the event e
   e.pre_proc=latest_proc[s->proc];//e becomes the latest event of its process
   e.post_proc.clear(); // e has no children
   switch (s->type)
      case ir::Trans::RD:
      {
       	  e.pre_mem = latest_global_rdwr;
       	  e.post_mem[e.val]=e.val;
      }
      case ir::Trans::WR:
      {
      	  e.pre_mem=latest_global_wr;
       	  std::vector<Event *>::iterator it;
       	  for (it=e.post_mem.begin();it<e.post_mem.end();it++)
          	  it=e.val;
       	  // set pre-readers = set of latest events which use the variable in all processes.
       	  //size of pre-readers is numbers of copies of the variable
       	  for (it=e.pre_readers.begin();it<e.pre_readers.end();it++)
       	  {
       		  int numofproc=gs.m.procs.size();
       		  for (i=0;i<numofproc;i++)
      		  it=latest_global_rdwr[i][s->addr]; // the latest global event of the process
       	  }
       }
       case ir::Trans::SYN:
       case ir::Trans::LOC:


}

} // end of namespace






