
#include "verbosity.h"

unsigned __verb_level = VERB_PRINT;

unsigned verb_debug = 0;
unsigned verb_trace = 0;
unsigned verb_info = 0;

void verb_set (int i)
{
	__verb_level = i;

#ifdef VERB_LEVEL_DEBUG
	verb_debug = i >= VERB_DEBUG;
#endif
#ifdef VERB_LEVEL_TRACE
	verb_trace = i >= VERB_TRACE;
#endif
#ifdef VERB_LEVEL_INFO
	verb_info = i >= VERB_INFO;
#endif
}

int verb_get ()
{
	return __verb_level;
}

void breakme (void)
{
}

