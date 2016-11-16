
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

/// internal function, this is a for loop that doubles acc until it is >= size
constexpr size_t __alignp2 (size_t size, size_t acc)
{
   return size <= acc ? acc : __alignp2 (size, acc << 1);
}
/// aligns size to the next (upper) power of 2
constexpr size_t alignp2 (size_t size)
{
   return __alignp2 (size, 1);
}

constexpr size_t __int2mask (size_t i, size_t m)
{
   return i <= m ? m : __int2mask (i, (m << 1) | 1);
}
constexpr size_t int2mask (size_t i)
{
   return __int2mask (i, 0);
}

constexpr unsigned __int2msb (size_t i, unsigned msb)
{
   return i == 0 ? msb : __int2msb (i >> 1, msb + 1);
}
constexpr unsigned int2msb (size_t i)
{
   return __int2msb (i >> 1, 0);
}


#endif
