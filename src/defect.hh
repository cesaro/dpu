
#ifndef _DEFECT_HH_
#define _DEFECT_HH_

#include <vector>
#include <string>

#include <llvm/Support/YAMLTraits.h>

#include "stid/executor.hh"
#include "unfolder/replay.hh"

namespace dpu
{

struct Defect
{
   Defect () :
      description (),
      replay ()
   {}

   Defect (const Unfolding &u) :
      description (),
      replay (Replay (u))
   {}

   Defect (std::string &des, const Unfolding &u) :
      description (des),
      replay (Replay (u))
   {}

   Defect (std::string &des, Replay &r) :
      description (des),
      replay (r)
   {}

   Defect (std::string &des, const Event *e) :
      description (des),
      replay (Replay (*e->unfolding(), e->cone))
   {}

   std::string description;
   stid::Replay replay;
};

} // namespace

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
struct llvm::yaml::MappingTraits<struct dpu::Defect> {
   static void mapping (llvm::yaml::IO &io, struct dpu::Defect &d)
   {
      io.mapRequired ("description", d.description);
      io.mapRequired ("replay", static_cast<stid::Replay::Vector&> (d.replay));
   }
};

#endif
