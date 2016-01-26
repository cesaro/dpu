
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
   std::vector<std::vector <Event *>> post_mem; // each vector of children events for a process
   std::vector<Event *> pre_readers; // only for WR events
   std::vector <std::vector <Event *>> post_wr; //write children of a write trans

   //only for RD and SYN events
   std::vector <Event *> post_rdsyn; // for RD and SYN events

   int                  val;
   std::vector<int>     localvals;

   ir::Trans *          trans;
   std::vector<Event *> cfl; // set of events in conflict

   Event (ir::Trans * t);


   ir::Trans & getTrans() {return *trans;}
   ir::Process & getProc() {return *(trans->proc);}
   void update (Config & c);
   void compute_cfl(Event & e);

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
   std::vector<Event *>             en;
   std::vector<Event *>             cex;


   Config();
   Config(Config & c);
   ir::State * getState() {return gstate;}
   void add(Event & e); // update the cut and the new event
};

class Unfolding
{
public:
   std::vector<Event> evt;
   std::vector <Event *> U;

   //methods
   void compute_en(Config & c);
   void compute_cex(Config & c);
   void explore(Config & C, std::vector<Event *> D, std::vector<Event *> A);
};


} // namespace pes

#endif 
