
#ifndef __PES_HH_
#define __PES_HH_

#include "ir.hh"

namespace pes
{
class Trans;
class State;

class Event
{
public:
   Event *              pre_proc;    // for all events
   std::vector<Event *> post_proc;

   Event *              pre_mem;     // for all events except LOCAL
   std::vector<Event *> post_mem; // for both RD and WR trans

   std::vector<Event *> pre_readers; // only for WR events

   uint32_t             val;
   std::vector<uint32_t>localvals;

   Trans *              trans;
   std::vector<Event *> cfl; // set of events in conflict

   bool in_U;  //bit to define the event in U or not

}; // end of class Event

/*
 *  class to represent a configuration in unfolding
 */
class Config
{
   std::vector <Event*> latest_proc; //size of vector = number of processes

   std::vector<Event*>  latest_global_wr; //size of vector = number of variable
   std::vector<std::vector<Event*>> latest_global_rdwr; //size =ProcessxVariable

   std::vector<Event*> latest_local_wr; // size=number of processes

   State * gstate;

public:
   Config();
   std::vector<Event *> compute_en();
   std::vector<Event *> compute_cex();
   void update(Event & e);
   void add(Event & e); // update the cut and the new event
};

class Unfolding
{
public:
   std::vector<Event> evt;

   //methods
   void extend(Config c);
   void explore(Config c,std::vector<Event *> d, std::vector<Event *> a);
};


} // namespace pes

#endif 
