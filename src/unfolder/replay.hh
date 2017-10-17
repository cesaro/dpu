
#ifndef _UNFOLDER_REPLAY_HH_
#define _UNFOLDER_REPLAY_HH_

#include <climits>

#include "stid/replay.hh"

#include "pes/unfolding.hh"
#include "unfolder/pidmap.hh"
#include "unfolder/trail.hh"
#include "pes/event.hh"
#include "pes/cut.hh"

#include "verbosity.h"

namespace dpu {

class Replay : public stid::Replay
{
public :

   /// Constructs a replay sequence suitable setting Steroids in FREE mode
   Replay (const Unfolding &u) :
      u (u)
   {
      finish ();
   }

   Replay (const Unfolding &u, const Cut &c) :
      u (u)
   {
      extend_from (c);
      finish ();
   }

   /// Constructs a replay sequence suitable for replaying the trail \p t.
   Replay (const Unfolding &u, const Trail &t, unsigned limit = UINT_MAX) :
      u (u)
   {
      extend_from (t, limit);
      finish ();
   }

   /// Constructs a replay sequence from a vector of integers. The
   /// vector shall **not** include the last step {-1, -1}
   Replay (const Unfolding &u, std::vector<int> &&v) :
      u (u)
   {
      ASSERT ((v.size() & 1) == 0);
      for (int i = 0; i < v.size(); i += 2) push_back ({v[i], v[i+1]});
      finish ();
   }

   /// Extends the replay vector with a sequence suitable to replay the trail
   inline void extend_from (const Trail &t, unsigned limit = UINT_MAX);

   /// Extends the replay vector with a replay sequence for the configuration c
   inline void extend_from (const Cut &c);

   /// Extends the replay vector with a replay sequence for the events in
   /// c1 \setminus c2; it assumes that c1 \cup c2 is a configuration
   inline void extend_from (const Cut &c1, const Cut &c2);

   /// Stores in the replay vector a suitable replay for the trail followed by
   /// J \setminus C
   inline void build_from (const Trail &t, const Cut &c, const Cut &j);

   /// Stores in the replay vector a sequence suitable to replay the trail
   inline void build_from (const Trail &t, unsigned limit = UINT_MAX);

   /// Stores in the replay vector a sequence suitable to replay \p c
   inline void build_from (const Cut &c);

   /// Adds the {-1, -1} event to the end of the replay vector
   inline void finish ();

   /// Returns a string depicting the replay sequence; altidx is the offset at
   /// which the replay sequence switches between the trail and the alternative
   inline std::string str (unsigned altidx = UINT_MAX);

   /// This object is a mapping from integers to integers. In the context of the
   /// Replay class, it is used as a map from DPU thread ids to Steroids thread
   /// ids. The mapping 0 -> 0 is always there, even after a call to clear().
   /// It is iteratively filled when transforming a configuration / trail / cut
   /// into a stid::Replay. Elsewhere (not in this class) it is also used in the
   /// opposite way, to store a mapping from Steroids tids to DPU tids. The
   /// scope of interpretation of the information contained in it is the
   /// represented execution. The Pidmap constructed during the transformation
   /// of 1 configuration cannot be used to map DPU tids to Steroid tids when
   /// translating a different configuration. (The same applies, by the way, in
   /// the opposite direction.). So any of the methods in this class that
   /// constructs or initializes the replay vector will clear() this map.
   Pidmap pidmap;
private :
   const Unfolding &u;
};

// implementation of inline methods
#include "unfolder/replay.hpp"

} // namespace

#endif
