#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <unistd.h>

#include "opts.hh"
#include "verbosity.h"

namespace dpu {
namespace opts {

// global variables storing the corresponding commandline options

const char *progname;
bool development;
int verbosity;
std::string inpath;
std::vector<const char *> argv;

void parse (int argc, char **argv_)
{
   int op;
	char *endptr;
	struct option longopts[] = {
			{"help", no_argument, nullptr, 'h'},
			{"verbosity", optional_argument, nullptr, 'v'},
			{"version", no_argument, nullptr, 'V'},
         //
			{"devel", no_argument, nullptr, 'd'},
			{"arg0", required_argument, nullptr, '0'},
			{0, 0, 0, 0}};

   // default options
   progname = "dpu";
   development = false;
	verbosity = VERB_PRINT;
   inpath = "";

	// parse the command line, supress automatic error messages by getopt
	opterr = 0;
	while (1) {
		op = getopt_long (argc, argv_, "0:vhV", longopts, nullptr);
		if (op == -1) break;
		switch (op) {
		case '0' :
         argv.push_back (optarg);
         break;
		case 'd' :
         development = true;
         break;
		case 'v' :
			if (! optarg || optarg[0] == 0) { verbosity++; break; }
			verbosity += strtol (optarg, &endptr, 10);
			if (*endptr != 0) usage ();
			break;
		case 'h' :
         help ();
		case 'V' :
         version ();
      default :
         usage ();
      }
   }

   // look for file.bc
	if (optind == argc) usage ();
   inpath = argv_[optind];

   // set the argv of the analyzed program
   for (; optind < argc; optind++) argv.push_back (argv_[optind]);

   // define an argv[0] if no program argument was given
   if (argv.empty()) argv.push_back (inpath.c_str());

   // set up the verbosity level
	if (verbosity < VERB_PRINT || verbosity > VERB_DEBUG) usage ();
	verb_set (verbosity);
}

void usage ()
{
   fprintf (stderr, "Usage: %s file.{bc,ll} ANALYZEROPTS -- PROGRAMOPTS\n", progname);
   fprintf (stderr, "Where ANALYZEROPTS can be:\n");
   fprintf (stderr, " -h, --help    Shows this message\n");
   fprintf (stderr, " -V, --version Displays version information\n");
   fprintf (stderr, " -v, --verb=N  Increments verbosity level by optional parameter N (1 to 3)\n");
   fprintf (stderr, "     --devel   For development purposes (calls internal tests)\n");

   exit (1);
}

void help ()
{
   usage ();
}

void version (void)
{
	PRINT ("v.0.1");
	exit (EXIT_SUCCESS);
}

void dump ()
{
   unsigned i;
   PRINT ("== begin arguments ==");
   PRINT (" development %d", development);
   PRINT (" verbosity   %d", verbosity);
   PRINT (" inpath      '%s'", inpath.c_str());
   PRINT (" argc        %d", argv.size());
   for (i = 0; i < argv.size(); i++)
      PRINT (" argv[%u]     '%s'", i, argv[i]);
   PRINT ("== end arguments ==");
}

}} // namespace
