#ifndef __REDBOX_HH_
#define __REDBOX_HH_

#include <iostream>

#include "pes/event-payload.hh"
#include "memory-pool.hh"
#include "misc.hh"

namespace dpu
{

class Redbox : public EventPayload
{
public :

   MemoryPool readpool;
   MemoryPool writepool;

   virtual ~Redbox()
   {}

   virtual size_t pointed_memory_size () const final
   {
      return readpool.pointed_memory_size() + writepool.pointed_memory_size();
   }

   virtual std::string str() const final
   {
      return fmt ("%d+%d", readpool.size(), writepool.size());
   }

   size_t size () const
   {
      return readpool.size() + writepool.size();
   }

   bool empty () const
   {
      return readpool.empty() and writepool.empty();
   }

   void dump () const
   {
      std::cout << "=== begin redbox ===\n";
      std::cout << "Read pool:\n";
      std::cout << readpool.str ();
      std::cout << "Write pool:\n";
      std::cout << writepool.str ();
      std::cout << "=== end redbox ===\n";
   }
};

inline
bool operator== (const Redbox &b1, const Redbox &b2)
{
   return b1.readpool == b2.readpool and b1.writepool == b2.writepool;
}

} // namespace dpu
#endif
