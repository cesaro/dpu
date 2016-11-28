

#ifndef __POR_HH_
#define __POR_HH_
#include "pes.hh"

namespace dpu
{
void basic_conf_to_replay (Unfolding &u, BaseConfig &c, std::vector<int> &replay);
void compute_cex(Unfolding & u, BaseConfig & c);

} //end of namespace
#endif
