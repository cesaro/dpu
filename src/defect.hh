
#ifndef _DEFECT_HH_
#define _DEFECT_HH_

#include <vector>
#include <string>

#include <llvm/Support/YAMLTraits.h>

#include "stid/executor.hh"

struct Defect
{
   std::string description;
   stid::Replay replay;
};

// yaml trait for a context switch
template<>
struct llvm::yaml::MappingTraits<struct replayevent> {
   static void mapping (llvm::yaml::IO &io, struct replayevent &x)
   {
      io.mapRequired ("tid", x.tid);
      io.mapRequired ("nr", x.count);
   }
   static const bool flow = true;
};

// yaml trait for the replay vector
LLVM_YAML_IS_SEQUENCE_VECTOR(struct replayevent);

// yaml trait for the defect
template<>
struct llvm::yaml::MappingTraits<struct Defect> {
   static void mapping (llvm::yaml::IO &io, struct Defect &d)
   {
      io.mapRequired ("description", d.description);
      io.mapRequired ("replay", static_cast<stid::Replay::Vector&> (d.replay));
   }
};

#endif
