/*
 * config.cc
 *
 *  Created on: Jan 11, 2016
 *      Author: tnguyen
 */
#include "config.h"

using std::map;
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
}

/*
 * function to add new event to a configuration and update necessary properties
 */
Config Config:: add(Event * e)
{

	Trans * s;
	s=e->getTrans();
	gstate=gstate.fire(s);
	switch (s)
	case s->type==RD: update_RD();
	case s->type==RD: update_RD();

}
} // end of namespace


