
#ifndef __PES_ACTION_HH_
#define __PES_ACTION_HH_

#include <cstdint>
#include <ostream>

// necessary for the .hpp
#include "verbosity.h"

namespace dpu
{

typedef uint64_t Addr;

enum class ActionType
{
   // loads
   RD8,
   RD16,
   RD32,
   RD64,
   // stores
   WR8,
   WR16,
   WR32,
   WR64,
   // memory management
   MALLOC,
   FREE,
   // threads
   THCREAT,
   THSTART,
   THEXIT,
   THJOIN,
   // locks
   //MTXINIT,
   MTXLOCK,
   MTXUNLK,
};

static std::ostream & operator<< (std::ostream &os, const enum ActionType &a);

const char *action_type_str (ActionType t);
const char *action_type_str (unsigned t);

/// Action (type, val)
struct Action
{
   ActionType type;
   Addr addr;
   uint64_t val;

   void pretty_print ();
   inline bool operator == (const Action &other) const;
};

// implementation of inline methods
#include "action.hpp"

} // namespace dpu
#endif
