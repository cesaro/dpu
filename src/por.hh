

#ifndef __POR_HH_
#define __POR_HH_
#include "pes.hh"

namespace dpu
{

bool is_conflict_free(std::vector<Event *> eset);
bool find_alternative (Config &c, std::vector<Event*> d, Config &J);
void enumerate_combination (unsigned i, std::vector<std::vector<Event *>> comb , std::vector<Event*> temp, Config &J);

} //end of namespace
#endif
