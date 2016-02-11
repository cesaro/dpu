
#include "verbosity.h"

int __verb_level = VERB_DEBUG;

int verb_debug = 1;
int verb_trace = 1;
int verb_info = 1;

void verb_set (int i)
{
	__verb_level = i;

	verb_debug = i >= VERB_DEBUG;
	verb_trace = i >= VERB_TRACE;
	verb_info = i >= VERB_INFO;
}

int verb_get ()
{
	return __verb_level;
}

void breakme (void)
{
}

