
#ifndef __PES_HH_
#define __PES_HH_

#include "ir.hh"

namespace pes
{

class Event
{
public:
   Event *              pre_proc;    // for all events
   std::vector<Event *> post_proc;

   Event *              pre_mem;     // for all events except LOCAL
   std::vector<Event *> post_mem; // for both RD and WR trans

   std::vector<Event *> pre_readers; // only for WR events

   int             val;
   std::vector<int>localvals;

   ir::Trans *          trans;
   std::vector<Event *> cfl; // set of events in conflict

   Event (ir::Trans * t);

   ir::Trans & getTrans() {return *trans;}
   ir::Process & getProc() {return *(trans->proc);}

}; // end of class Event

/*
 *  class to represent a configuration in unfolding
 */
class Config
{
public:
   ir::State * gstate;

   Config();
   Config(Config & c);
   Config & add(Event & e); // update the cut and the new event
   ir::State * getState() {return gstate;}
};

class Unfolding
{
public:
   std::vector<Event> evt;
   std::vector <Event *> U;
   std::vector <Event*> latest_proc; //latest events of all processes
   std::vector<Event*>  latest_global_wr; //size of vector = number of variable
   std::vector<std::vector<Event*>> latest_global_rdwr; //size =ProcessxVariable
   std::vector<Event*> latest_local_wr; // size=number of processes

   //methods
   std::vector<Event *> compute_en(const Config & c);
   std::vector<Event *> compute_cex(const Config & c);
   void extend(const Config & c);
   void explore(Config & C, std::vector<Event *> D, std::vector<Event *> A);
};


} // namespace pes

#endif 
