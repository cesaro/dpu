
#ifndef _C15U_DEFECT_HH_
#define _C15U_DEFECT_HH_

#include <vector>
#include <string>

#include <llvm/Support/YAMLTraits.h>

#include "stid/executor.hh"

struct Defect
{
   std::string description;
   std::vector<struct replayevent> replay;
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
      io.mapRequired ("replay", d.replay);
   }
};

#endif
