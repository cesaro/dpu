#ifndef __UNFOLDER_INTERVAL_HH_
#define __UNFOLDER_INTERVAL_HH_

#include "verbosity.h"
#include "misc.hh"

namespace dpu
{

template<typename Addr = char *>
class MemoryRegion
{
public:
   Addr lower;
   Addr upper;

   /// Default constructor, necessary to resize containers
   MemoryRegion () :
      lower (0),
      upper (0)
   {}

   MemoryRegion (Addr base, size_t size) :
      lower (base),
      upper (base + size)
   {}

   /// Returns true iff the intersection of *this and \p other is non-empty.
   bool overlaps (const MemoryRegion<Addr> &other) const
   {
      ASSERT (lower <= upper);
      ASSERT (other.lower <= other.upper);
      return 
         other.lower < upper and
         lower < other.upper and
         lower != upper and
         other.lower != other.upper;
   }

   /// Returns the intersection of *this and \p other. Could be empty.
   MemoryRegion<Addr> intersection (const MemoryRegion<Addr> &other) const
   {
      Addr l = std::max (lower, other.lower);
      Addr u = std::min (upper, other.upper);
      if (l < u)
         return MemoryRegion<Addr> (l, u - l);
      else
         return MemoryRegion<Addr> (); // empty interval
   }

   bool is_adjacent_below (const MemoryRegion<Addr> &other) const
   {
      return upper == other.lower;
   }
   bool is_adjacent_above (const MemoryRegion<Addr> &other) const
   {
      return other.upper == lower;
   }
   bool is_adjacent (const MemoryRegion<Addr> &other) const
   {
      return is_adjacent_below (other) or is_adjacent_above (other);
   }

   size_t size () const { return upper - lower; }
   bool empty () const { return lower == upper; }

   std::string str () const
   {
      return fmt ("[%#16zx %#16zx (%d)]", lower, upper, size());
   }
};

template<typename Addr>
bool operator== (const MemoryRegion<Addr> &a, const MemoryRegion<Addr> &b)
{
   ASSERT (a.lower <= a.upper);
   ASSERT (b.lower <= b.upper);
   return a.lower == b.lower and a.upper == b.upper;
}

template<typename Addr>
bool operator< (const MemoryRegion<Addr> &a, const MemoryRegion<Addr> &b)
{
   ASSERT (a.lower <= a.upper);
   ASSERT (b.lower <= b.upper);
   return a.upper <= b.lower;
}

template<typename Addr>
bool operator> (const MemoryRegion<Addr> &a, const MemoryRegion<Addr> &b)
{
   return b < a;
}

} // namespace dpu
#endif
