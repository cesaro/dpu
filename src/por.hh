

#ifndef __POR_HH_
#define __POR_HH_
#include "pes.hh"

namespace dpu
{

void cut_to_replay (Unfolding &u, Cut &c, std::vector<int> &replay);
void compute_cex (Unfolding &u, Config &c);
//bool check_cfl_same_tree(int idx, const Event & e1, const Event & e2);
//bool check_2difs(Event & e1, Event & e2);
//bool check_cfl(Event & e1, Event & e2 );
void find_an_alternative(Config & C, std::vector<Event *> D, std::vector<Event *> & J, std::vector<Event *> & A );
void compute_alt(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> & J, std::vector<Event *> & A);

} //end of namespace
#endif
