#ifndef _OPTS_HH_
#define _OPTS_HH_

#include <string>
#include <vector>

#include "c15u/c15unfold.hh"

namespace dpu {
namespace opts {

extern const char *progname;
extern bool development;
extern int verbosity;
extern std::string inpath;
extern std::vector<const char *> argv;
extern unsigned kbound;
extern C15unfolder::Alt_algorithm alt_algo;
extern std::string dotpath;


void parse (int argc, char **argv_);
void dump ();
void usage ();
void help ();
void version ();

}} // namespace dpu::opts

#endif
