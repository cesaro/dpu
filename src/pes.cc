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
Config Config::add(Event & e)
{
   Config c;
   ir::Trans * s=e.getTrans();
   ir::State & gs=*gstate;
   gstate=s->fire(gs); //update new global state

   e.pre_proc=latest_proc[s->proc];//e becomes the latest event of its process
   latest_proc[s->proc]=e;
   e.post_proc.clear(); // e has no children

   switch (s->type)
      case ir::Trans::RD:
      {
    	  e.pre_mem=latest_global_rdwr;


      }
      case ir::Trans::WR: update_WR();
      case ir::Trans::SYN: update_SYN();
      case ir::Trans::LOC: update_LOC();
   return c;
}

} // end of namespace






