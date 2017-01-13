#ifndef _OPTS_HH_
#define _OPTS_HH_

#include <string>
#include <vector>

namespace dpu {
namespace opts {

extern bool development;
extern int verbosity;
extern std::string inpath;
extern std::vector<const char *> argv;

void parse (int argc, char **argv_);
void dump ();
void usage ();
void help ();
void version ();

}} // namespace dpu::opts

#endif
