#ifndef __PES_HH_
#define __PES_HH_

#include <iostream>



#include "ir.hh"
#include "pes.hh"


namespace ir
{
class Event;
class State;
class Process;

class Config
{
	std::map<Process*,Event*> latest_proc;
	std::map<int, Event*> latest_global_wr;
	std::map<std::pair<Process*,int>, Event*> latest_global_rdwr;
	std::map<int, Event*> latest_local_wr;
	State gstate;
public:
	Config();
	std::vector<Event *> compute_en();
	std::vector<Event *> compute_cex();
	Config add(Event * e); // creat new Config and update the cut

};
} //end of namespace

#endif
