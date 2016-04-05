
#ifndef __MISC_HH_
#define __MISC_HH_

#include <string>

std::string fmt (const std::string fmt_str, ...);
std::string operator * (std::string lhs, int i);

std::string quoted_str (const char * str);

#endif
