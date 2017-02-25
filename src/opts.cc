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
unsigned kbound;
C15unfolder::Alt_algorithm alt_algo;
std::string dotpath;
std::string instpath;
size_t memsize;
size_t stacksize;

void parse (int argc, char **argv_)
{
   int op, i;
	char *endptr;
	struct option longopts[] = {
			{"help", no_argument, nullptr, 'h'},
			{"verbosity", optional_argument, nullptr, 'v'},
			{"version", no_argument, nullptr, 'V'},
         //
			{"devel", no_argument, nullptr, 'd'},
			{"alt", required_argument, nullptr, 'a'},
			{"arg0", required_argument, nullptr, '0'},
			{"dot", required_argument, nullptr, 'D'},
			{"mem", required_argument, nullptr, 'm'},
			{"stack", required_argument, nullptr, 's'},
			{"dump-instrumented", required_argument, nullptr, 'i'},
			{0, 0, 0, 0}};

   // default options
   progname = "dpu";
   development = false;
	verbosity = VERB_PRINT;
   inpath = "";
   dotpath = "";
   //alt_algo = C15unfolder::Alt_algorithm::KPARTIAL;
   alt_algo = C15unfolder::Alt_algorithm::ONLYLAST;
   kbound = 1;
   memsize = CONFIG_GUEST_DEFAULT_MEMORY_SIZE;
   stacksize = CONFIG_GUEST_DEFAULT_THREAD_STACK_SIZE;

	// parse the command line, supress automatic error messages by getopt
	opterr = 0;
	while (1) {
		op = getopt_long (argc, argv_, "0:a:vhVm:s:", longopts, nullptr);
		if (op == -1) break;
		switch (op) {
		case '0' :
         argv.push_back (optarg);
         break;
		case 'd' :
         development = true;
         break;
		case 'a' :
			i = strtol (optarg, &endptr, 10);
         if (i < -1) usage(1);
         switch (i) {
         case -1 :
            alt_algo = C15unfolder::Alt_algorithm::SDPOR;
            break;
         case 0 :
            alt_algo = C15unfolder::Alt_algorithm::OPTIMAL;
            break;
         case 1 :
            //alt_algo = C15unfolder::Alt_algorithm::KPARTIAL;
            alt_algo = C15unfolder::Alt_algorithm::ONLYLAST;
            kbound = 1;
            break;
         default :
            alt_algo = C15unfolder::Alt_algorithm::KPARTIAL;
            kbound = i;
            break;
         }
         break;
      case 'm' :
         memsize = parse_size (optarg, 'M');
         if (memsize == SIZE_MAX) usage (1);
         break;
      case 's' :
         stacksize = parse_size (optarg, 'M');
         if (stacksize == SIZE_MAX) usage (1);
         break;
		case 'D' :
			dotpath = optarg;
         break;
		case 'i' :
			instpath = optarg;
         break;
		case 'v' :
			if (! optarg || optarg[0] == 0) { verbosity++; break; }
			verbosity += strtol (optarg, &endptr, 10);
			if (*endptr != 0) usage (1);
			break;
		case 'h' :
         help ();
		case 'V' :
         version ();
      default :
         usage (1);
      }
   }

   // look for file.bc
	if (optind == argc) usage (1);
   inpath = argv_[optind];
   optind++;

   // set the argv of the analyzed program
   for (; optind < argc; optind++) argv.push_back (argv_[optind]);

   // define an argv[0] if no program argument was given
   if (argv.size() == 0) argv.push_back (inpath.c_str());

   // set up the verbosity level
	if (verbosity < VERB_PRINT || verbosity > VERB_DEBUG) usage (1);
	verb_set (verbosity);
}

size_t parse_size (const char *str, char default_units)
{
   char *endptr;
   char units;
   size_t s;
   
   s = strtoul (str, &endptr, 10);
   units = *endptr ? *endptr : default_units;
   switch (units)
   {
   case 0 :
      return s;
   case 'k' :
   case 'K' :
      return s * 1024ul;
   case 'm' :
   case 'M' :
      return s * 1024ul * 1024ul;
   case 'g' :
   case 'G' :
      return s * 1024ul * 1024ul * 1024ul;
   default :
      return SIZE_MAX;
   }
}

void help ()
{
   usage (0);
}

void usage (int exitcode)
{
   fprintf (stderr, "Usage: %s FILE.{c,bc,ll} ANALYZEROPTS -- PROGRAMOPTS\n", progname);
   fprintf (stderr, "Where ANALYZEROPTS can be:\n");
   print_options();
   exit (exitcode);
}

void print_options ()
{
   fprintf (stderr, " -h,   --help             shows this message\n");
   fprintf (stderr, " -V,   --version          displays version information\n");
   fprintf (stderr, " -v,   --verb=N           increments verbosity level by optional parameter N (1 to 3)\n");
   fprintf (stderr, "       --devel            for internal use (calls internal tests)\n");
   fprintf (stderr, "       --gdb              starts a gdb session with the backend\n");
   fprintf (stderr, "       --dot=PATH         dumps DOT for full infolding into PATH\n");
   //fprintf (stderr, " -a {0,1,K}, --alt={0,1,K} alternatives: 0 optimal, 1 only-last, K K-partial (default 1)\n");
   fprintf (stderr, " -a K, --alt=K            alternatives: K=0 -> optimal, K>=1 -> K-partial (default 1)\n");
   fprintf (stderr, " -m N, --mem=N            sets the guest memory, in MB (default %zuM)\n", memsize / (1 << 20));
   fprintf (stderr, " -s N, --stack=N          sets default size for thread stacks, in MB (default %zuM)\n", stacksize / (1 << 20));
   fprintf (stderr, "       --dump-instr=PATH  dumps instrumented LLVM bytecode to PATH\n");
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
   PRINT (" development    %d", development);
   PRINT (" verbosity      %d", verbosity);
   PRINT (" inpath         '%s'", inpath.c_str());
   PRINT (" argc           %d", argv.size());
   for (i = 0; i < argv.size(); i++)
      PRINT (" argv[%u]        '%s'", i, argv[i]);
   switch (alt_algo) {
   case C15unfolder::Alt_algorithm::KPARTIAL :
      PRINT (" alt            %u-partial", kbound);
      break;
   case C15unfolder::Alt_algorithm::OPTIMAL :
      PRINT (" alt            optimal");
      break;
   case C15unfolder::Alt_algorithm::ONLYLAST :
      PRINT (" alt            only-last");
      break;
   case C15unfolder::Alt_algorithm::SDPOR :
      PRINT (" alt            sdpor");
      break;
   }
   PRINT (" dot            '%s'", dotpath.c_str());
   PRINT (" dump-instr     '%s'", instpath.c_str());
   PRINT (" memory         %zu%s", UNITS_SIZE(memsize), UNITS_UNIT(memsize));
   PRINT (" thread stacks  %zu%s", UNITS_SIZE(stacksize), UNITS_UNIT(stacksize));
   PRINT ("== end arguments ==");
}

}} // namespace
