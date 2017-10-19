#ifndef _OPTS_HH_
#define _OPTS_HH_

#include <string>
#include <vector>

#include "unfolder/alt-algorithm.hh"

namespace dpu {
namespace opts {

typedef enum {POR, DRA} Analysis;

extern const char *progname;
extern int verbosity;
extern std::string inpath;
extern std::string defectspath;
extern Analysis analysis;
extern std::vector<const char *> argv;
extern unsigned kbound;
extern Altalgo alt_algo;
extern std::string dotpath;
extern std::string instpath;
extern size_t memsize;
extern size_t stacksize;
extern unsigned optlevel;
extern unsigned maxcts;
extern bool strace;
extern bool dosleep;
extern unsigned timeout;
extern unsigned drfreq;


void parse (int argc, char **argv_);
size_t parse_size (const char *str, char default_units);
void dump ();
void usage (int exitcode);
void print_options ();
void help ();
void version ();

}} // namespace dpu::opts

#endif
