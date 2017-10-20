#ifndef __REDBOXFACTORY_HH_
#define __REDBOXFACTORY_HH_

#include <vector>

#include "pes/action.hh"

#include "redbox.hh"
#include "verbosity.h"

namespace dpu
{

class RedboxFactory
{
public:

   const unsigned INITIAL_CAPACITY = 250 * 1000;

   RedboxFactory () :
      read_regions (),
      write_regions ()
   {
      read_regions.reserve (INITIAL_CAPACITY);
      write_regions.reserve (INITIAL_CAPACITY);
   }

   ~RedboxFactory ()
   {
      for (auto b : boxes) delete b;
   }

   inline void add (Addr addr, ActionType t);
   inline void add_read (Addr addr, unsigned size);
   inline void add_write (Addr addr, unsigned size);
   inline void clear ();

   inline Redbox *create ();

   bool empty () const
   {
      return read_regions.empty() and write_regions.empty();
   }

   size_t size() const
   {
      return read_size() + write_size();
   }

   size_t read_size() const
   {
      return read_regions.size();
   }

   size_t write_size() const
   {
      return write_regions.size();
   }

private:
   MemoryPool::Container read_regions;
   MemoryPool::Container write_regions;
   std::vector<Redbox*> boxes;

   inline void compress (MemoryPool::Container &regions);
};

// implementation of inline methods
#include "redbox-factory.hpp"

} // namespace dpu
#endif
