
#ifndef __PES_HH_
#define __PES_HH_

#include "ir.hh"

namespace pes
{

class Event
{
public:
   Event *              pre_proc;    // for all events, predecessor in the same process
   std::vector<Event *> post_proc;  // set of successors in the same process

   Event *              pre_mem;     // parent of the event, for all events except LOCAL,

   // only for WR events
   std::vector<vector <Event *>> post_mem; // each vector of events children for a process
   std::vector<Event *> pre_readers; // only for WR events
   std::vector <std::vector <Event *>> post_wr; //write children of a written trans

   //only for RD events
   std::vector <Event *> post_rd;
   int                  val;
   std::vector<int>     localvals;

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
   ir::State *                      gstate;
   std::vector <Event*>             latest_proc; //latest events of all processes
   std::vector<Event*>              latest_global_wr; //size of vector = number of variable
   std::vector<std::vector<Event*>> latest_global_rdwr; //size =ProcessxVariable
   std::vector<Event*>              latest_local_wr; // size=number of processes


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

   //methods
   std::vector<Event *> compute_en(const Config & c);
   std::vector<Event *> compute_cex(const Config & c);
   void extend(const Config & c);
   std::vector <Event * > compute_cfl(Event & e);
   void explore(Config & C, std::vector<Event *> D, std::vector<Event *> A);
};


} // namespace pes

#endif 
