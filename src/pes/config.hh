#ifndef __PES_CONFIG_HH_
#define __PES_CONFIG_HH_

#include <unordered_map>

#include "pes/unfolding.hh"
#include "pes/event.hh"

#include "verbosity.h"

namespace dpu
{

class Config : public Cut
{
public:

   /// creates an empty onfiguration for as much as u.num_procs processes
   inline Config (const Unfolding &u);
   /// creates an empty configuration for as much as n processes
   inline Config (unsigned n);
   /// copy constructor
   inline Config (const Config &other);

   /// assignment operator
   inline Config & operator= (const Config & other);
   /// move-assignment operator
   inline Config & operator= (Config && other);
   
   /// fires an enabled event
   inline void fire (Event *e);
   /// removes one of the maximal events of the configuration
   inline void unfire (Event *e);

   /// empties the configuration
   inline void clear ();

   /// prints the cut in stdout
   void dump () const;

   /// maximal event for the given pid, or nullptr
   inline Event *proc_max (unsigned pid);
   /// maximal event for the given address (or THCREAT/EXIT if pid is given)
   inline const Event * mutex_max (Addr a) const;
   inline Event *mutex_max (Addr a);
   
public:
   /// map from lock addresss to events
   std::unordered_map<Addr,Event*> mutexmax;

   void __dump_mutexes () const;
};

// implementation of inline methods
#include "pes/config.hpp"

} // namespace dpu
#endif
