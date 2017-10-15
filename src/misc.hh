
#ifndef __MISC_HH_
#define __MISC_HH_

#include <string>

#define UNITS_UNIT(s) \
      ((s) < 2048 ? "B" : \
         (s) / 1024 < 1024 ? "K" : \
            (s) / (1024 * 1024) < 1024 ? "M" : "G")

#define UNITS_SIZE(s) \
      ((s) < 2048 ? (s) : \
         (s) / 1024 < 1024 ? (s) / 1024 : \
            (s) / (1024 * 1024) < 1024 ? (s) / (1024 * 1024) : \
               (s) / (size_t) (1024 * 1024 * 1024))

std::string fmt (const std::string fmt_str, ...);
std::string operator * (std::string lhs, int i);

std::string quoted_str (const char * str);


#endif
