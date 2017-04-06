
#ifndef _UTIL_MISC_H_
#define _UTIL_MISC_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

// clean state
#ifdef DEBUG
#undef DEBUG
#undef TRACE
#undef INFO
#undef PRINT
#undef BREAK
#undef ASSERT
#endif

// the different levels of verbosity in the program
#define VERB_DEBUG   3
#define VERB_TRACE   2
#define VERB_INFO    1
#define VERB_PRINT   0

// the "verbosity" of the program; initially set to VERB_DEBUG
extern unsigned __verb_level;

// these evaluate to true or false depending on the current verbosity level
extern unsigned verb_debug;
extern unsigned verb_trace;
extern unsigned verb_info;

// setting and getting the verbosity level
void verb_set (int i);
int verb_get ();

// we define macros according to the maximum level of information the user wants
// to see
#if CONFIG_MAX_VERB_LEVEL >= 3
#define VERB_LEVEL_DEBUG
#endif
#if CONFIG_MAX_VERB_LEVEL >= 2
#define VERB_LEVEL_TRACE
#endif
#if CONFIG_MAX_VERB_LEVEL >= 1
#define VERB_LEVEL_INFO
#endif

#define MYLOG(level,fmt,args...) \
   do { \
      if (level <= CONFIG_MAX_VERB_LEVEL && level <= __verb_level) \
      { \
         printf (fmt, ##args); \
      } \
   } while (0)

// the actual primitives you should use, with and without new line
#define DEBUG(fmt,args...)    MYLOG (3, fmt "\n", ##args)
#define TRACE(fmt,args...)    MYLOG (2, fmt "\n", ##args)
#define INFO(fmt,args...)     MYLOG (1, fmt "\n", ##args)
#define PRINT(fmt,args...)    MYLOG (0, fmt "\n", ##args)

#define DEBUG_(fmt,args...)   MYLOG (3, fmt, ##args)
#define TRACE_(fmt,args...)   MYLOG (2, fmt, ##args)
#define INFO_(fmt,args...)    MYLOG (1, fmt, ##args)
#define PRINT_(fmt,args...)   MYLOG (0, fmt, ##args)

// remove at compile time primitives that will never be active
#ifndef VERB_LEVEL_DEBUG
#undef DEBUG
#undef DEBUG_
#define DEBUG(fmt,args...) {}
#define DEBUG_(fmt,args...) {}
#endif

#ifndef VERB_LEVEL_TRACE
#undef TRACE
#undef TRACE_
#define TRACE(fmt,args...) {}
#define TRACE_(fmt,args...) {}
#endif

#ifndef VERB_LEVEL_INFO
#undef INFO
#undef INFO_
#define INFO(fmt,args...) {}
#define INFO_(fmt,args...) {}
#endif

// more debugging primitives
void breakme (void);
#define BREAK(expr) if (expr) breakme ()
#ifdef CONFIG_DEBUG
#define ASSERT(expr) \
   {if (! (expr)) { \
      PRINT (__FILE__ ":%d: %s: Assertion `" #expr "' failed.\n", \
            __LINE__, __func__); \
      fflush (stdout); \
      fflush (stderr); \
      breakme (); \
      exit (1); \
   }}
#else
#define ASSERT(expr)
#endif
#define DEBUG2(fmt,args...) \
   DEBUG (__FILE__ ":%d: %s: " fmt, __LINE__, __func__, ##args)
#define SHOW(expr,type) DEBUG2 (#expr "='%" type "'", expr)


// additional stuff for C++
#ifdef __cplusplus
} // extern "C"

//#include <string>
//std::string fmt (const std::string fmt_str, ...);

// FIXME uncomment this
/*inline void mylog (int level, const std::string & s)
   { mylog (level, "%s", s.c_str ()); } */

#endif // __cplusplus

#endif // _UTIL_MISC_H_
