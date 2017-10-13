#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <unistd.h>

#include "opts.hh"
#include "verbosity.h"
#include "misc.hh"

namespace dpu {
namespace opts {

// global variables storing the corresponding commandline options

const char *progname;
int verbosity;
std::string inpath;
std::vector<const char *> argv;
unsigned kbound;
Altalgo alt_algo;
std::string dotpath;
std::string instpath;
std::string defectspath;
size_t memsize;
size_t stacksize;
unsigned optlevel;
unsigned maxcts;
bool strace;
bool dosleep;
unsigned timeout;

void parse (int argc, char **argv_)
{
   int op, i;
   unsigned u;
	char *endptr;
	struct option longopts[] = {
			{"help", no_argument, nullptr, 'h'},
			{"verbosity", optional_argument, nullptr, 'v'},
			{"version", no_argument, nullptr, 'V'},
         //
			{"alt", required_argument, nullptr, 'a'},
			{"arg0", required_argument, nullptr, '0'},
			{"dot", required_argument, nullptr, 'D'},
			{"mem", required_argument, nullptr, 'm'},
			{"stack", required_argument, nullptr, 's'},
			{"dump-instrumented", required_argument, nullptr, 'i'},
			{"strace", no_argument, nullptr, 'S'},
			{"dosleep", no_argument, nullptr, 'L'},
			{"maxcts", required_argument, nullptr, 'x'},
			{"timeout", required_argument, nullptr, 't'},
			{0, 0, 0, 0}};

   // default options
   progname = "dpu";
	verbosity = VERB_PRINT;
   inpath = "";
   dotpath = "";
   defectspath = "./defects.yml";
   //alt_algo = Altalgo::KPARTIAL;
   alt_algo = Altalgo::ONLYLAST;
   kbound = 1;
   memsize = CONFIG_GUEST_DEFAULT_MEMORY_SIZE;
   stacksize = CONFIG_GUEST_DEFAULT_THREAD_STACK_SIZE;
   optlevel = 1;
   maxcts = UINT_MAX;
   strace = false;
   dosleep = false;
   timeout = 0;

	// parse the command line, supress automatic error messages by getopt
	opterr = 0;
	while (1) {
		op = getopt_long (argc, argv_, "0:a:vhVm:s:O:x:", longopts, nullptr);
		if (op == -1) break;
		switch (op) {
		case '0' :
         argv.push_back (optarg);
         break;
		case 'a' :
			i = strtol (optarg, &endptr, 10);
         if (i < -1) usage(1);
         switch (i) {
         case -1 :
            alt_algo = Altalgo::SDPOR;
            break;
         case 0 :
            alt_algo = Altalgo::OPTIMAL;
            break;
         case 1 :
            //alt_algo = Altalgo::KPARTIAL;
            alt_algo = Altalgo::ONLYLAST;
            kbound = 1;
            break;
         default :
            alt_algo = Altalgo::KPARTIAL;
            kbound = i;
            break;
         }
         break;
      case 'O' :
			u = strtoul (optarg, &endptr, 10);
			if (*endptr != 0) usage (1);
         if (u > 3) usage(1);
         optlevel = u;
         break;
      case 'x' :
			u = strtoul (optarg, &endptr, 10);
			if (*endptr != 0) usage (1);
         maxcts = u;
         break;
      case 't' :
			u = strtoul (optarg, &endptr, 10);
			if (*endptr != 0) usage (1);
         timeout = u;
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
		case 'S' :
			strace = true;
         break;
		case 'L' :
			dosleep = true;
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
   if (endptr[0] and endptr[1]) return SIZE_MAX; // eg. "12xxx"
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
   fprintf (stderr, "Usage: %s FILE.{c,i,bc,ll} ANALYZEROPTS -- PROGRAMOPTS\n", progname);
   fprintf (stderr, "Where ANALYZEROPTS can be:\n");
   print_options();
   exit (exitcode);
}

void print_options ()
{
   fprintf (stderr, " -h,   --help             shows this message\n");
   fprintf (stderr, " -V,   --version          displays version information\n");
   fprintf (stderr, " -v,   --verb=N           increments verbosity level by optional parameter N (1 to 3)\n");
   fprintf (stderr, "       --gdb              runs the backend in a gdb session\n");
   fprintf (stderr, "       --callgrind        runs the backend in a callgrind session\n");
   fprintf (stderr, "       --dot=PATH         dumps DOT for full unfolding into PATH\n");
   fprintf (stderr, " -D MACRO                 defines a preprocessor macro\n");
   //fprintf (stderr, " -a {0,1,K}, --alt={0,1,K} alternatives: 0 optimal, 1 only-last, K K-partial (default 1)\n");
   fprintf (stderr, " -a K, --alt=K            alternatives: K=0 -> optimal, K>=1 -> K-partial (default 1)\n");
   fprintf (stderr, " -m N, --mem=N            sets the guest memory, in MB (default %zuM)\n", memsize / (1 << 20));
   fprintf (stderr, " -s N, --stack=N          sets default size for thread stacks, in MB (default %zuM)\n", stacksize / (1 << 20));
   fprintf (stderr, "       --dump-instr=PATH  dumps instrumented LLVM bytecode to PATH\n");
   fprintf (stderr, " -O L                     optimization level (0 to 3) (default %u)\n", optlevel);
   fprintf (stderr, "       --strace           prints strace(1)-like info on program execution (default %d)\n", strace);
   fprintf (stderr, "       --dosleep          makes sleep(3) not to return EINTR immediately (default %d)\n", dosleep);
   fprintf (stderr, " -x N  --maxcts N         prune POR tree beyond N context switches (default: no limit)\n");
   fprintf (stderr, "       --timeout N        stop exploration after N seconds\n");
}

void version (void)
{
	PRINT ("dpu %s (%s%s), compiled %s",
         CONFIG_VERSION,
         CONFIG_BUILD_COMMIT,
         CONFIG_BUILD_DIRTY ? ", dirty" : "",
         CONFIG_BUILD_DATE);
#ifdef CONFIG_DEBUG
	PRINT ("Build type: debug");
#endif
#ifdef CONFIG_RELEASE
	PRINT ("Build type: release");
#endif
   PRINT_ ("Features: ");
#ifdef CONFIG_STATS_DETAILED
   PRINT_ ("+detailed-stats ");
#else
   PRINT_ ("-detailed-stats ");
#endif
   PRINT ("");
   PRINT ("Event structure: %zu slots, up to %zu events/slot, %zu%s memory per slot, "
         "%zu%s total memory, aligned to %zu%s, skip-step %u",
         Unfolding::MAX_PROC,
         Unfolding::PROC_SIZE / sizeof (Event),
         UNITS_SIZE (Unfolding::PROC_SIZE),
         UNITS_UNIT (Unfolding::PROC_SIZE),
         UNITS_SIZE (Unfolding::PROC_SIZE * Unfolding::MAX_PROC),
         UNITS_UNIT (Unfolding::PROC_SIZE * Unfolding::MAX_PROC),
         UNITS_SIZE (Unfolding::ALIGN),
         UNITS_UNIT (Unfolding::ALIGN),
         CONFIG_SKIP_STEP);

   PRINT ("Trace buffer size: : %.1f Mevents",
         CONFIG_GUEST_TRACE_BUFFER_SIZE / 1000000.0);
   PRINT ("Maximum verbosity level: %d", CONFIG_MAX_VERB_LEVEL);
	PRINT ("Compilation: %s", CONFIG_COMPILE);
	//PRINT ("Linking: %s", CONFIG_LINK);
	exit (EXIT_SUCCESS);
}

void dump ()
{
   unsigned i;
   PRINT ("== begin arguments ==");
   PRINT (" verbosity      %d", verbosity);
   PRINT (" inpath         '%s'", inpath.c_str());
   PRINT (" argc           %zu", argv.size());
   for (i = 0; i < argv.size(); i++)
      PRINT (" argv[%u]        '%s'", i, argv[i]);
   switch (alt_algo) {
   case Altalgo::KPARTIAL :
      PRINT (" alt            %u-partial", kbound);
      break;
   case Altalgo::OPTIMAL :
      PRINT (" alt            optimal");
      break;
   case Altalgo::ONLYLAST :
      PRINT (" alt            only-last");
      break;
   case Altalgo::SDPOR :
      PRINT (" alt            sdpor");
      break;
   }
   PRINT (" dot            '%s'", dotpath.c_str());
   PRINT (" dump-instr     '%s'", instpath.c_str());
   PRINT (" memory         %zu%s", UNITS_SIZE(memsize), UNITS_UNIT(memsize));
   PRINT (" thread stacks  %zu%s", UNITS_SIZE(stacksize), UNITS_UNIT(stacksize));
   PRINT (" optlevel       %u", optlevel);
   PRINT (" maxcts         %u", maxcts);
   PRINT (" strace         %u", strace);
   PRINT (" dosleep        %u", dosleep);
   PRINT ("== end arguments ==");
}

}} // namespace
