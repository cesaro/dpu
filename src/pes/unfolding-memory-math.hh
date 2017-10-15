
// UnfoldingMemoryConstants needs to be defined immediately after Event, but
// before the implementation of its inline methods
#include "pes/event.hh"

#ifndef __PES_UNFOLDINGMEMORYMATH_HH_
#define __PES_UNFOLDINGMEMORYMATH_HH_

#include "config.h"

namespace dpu
{

/// Internal, this is a for loop that doubles acc until it is >= size
inline constexpr size_t __alignp2 (size_t size, size_t acc)
{
   return size <= acc ? acc : __alignp2 (size, acc << 1);
}

/// Internal, see below.
inline constexpr size_t __int2mask (size_t i, size_t m)
{
   return i <= m ? m : __int2mask (i, (m << 1) | 1);
}

/// Internal, see below.
inline constexpr unsigned __int2msb (size_t i, unsigned msb)
{
   return i == 0 ? msb : __int2msb (i >> 1, msb + 1);
}

/// Returns the next (upper) power of two of \p size
inline constexpr size_t alignp2 (size_t size)
{
   return __alignp2 (size, 1);
}

/// Returns the smallest bit mask m that covers \p i (that is, i <= m)
/// where m is of the form (2^n - 1).
inline constexpr size_t int2mask (size_t i)
{
   return __int2mask (i, 0);
}

/// Returns the weight of the most significant bit of \p i.
inline constexpr unsigned int2msb (size_t i)
{
   return __int2msb (i >> 1, 0);
}

/// Constants and memory arithmetic for the internal data structures of the
/// Unfolding class. Needs to be defined in a separate class due to circularity
/// in the data types around the Unfolding class (Event, Eventbox, Process).
class UnfoldingMemoryMath
{
public:

   /// Maximum number of processes in the unfolding.
   static constexpr size_t MAX_PROC = CONFIG_MAX_PROCESSES;

   /// Approximate maximum size (in bytes) used for the events of one process.
   static constexpr size_t PROC_SIZE =
      alignp2 (CONFIG_MAX_EVENTS_PER_PROCCESS * sizeof (Event));

   /// alignment for the raw memory pool of the entire unfolding
   static constexpr size_t ALIGN = PROC_SIZE * alignp2 (MAX_PROC);

   /// Extracts the pid to which an object belongs (Process, EventBox, Event)
   static unsigned ptr2pid (const void *ptr)
   {
      return (((size_t) ptr) >> int2msb (PROC_SIZE)) & int2mask (MAX_PROC - 1);
   }

   /// Returns a unique id (within the process) for the pointer
   static unsigned ptr2puid (const void *ptr)
   {
      return int2mask (PROC_SIZE - 1) & (size_t) ptr;
   }

   /// Returns a unique id (globally) for the pointer
   static unsigned ptr2uid (const void *ptr)
   {
      return int2mask (ALIGN - 1) & (size_t) ptr;
   }

   /// Returns the Process of any pointer in that process' address space
   static Process *ptr2proc (const void *ptr)
   {
      return (Process *) (((size_t) ptr) & ~int2mask (PROC_SIZE - 1));
   }
};

} // namespace dpu
#endif
