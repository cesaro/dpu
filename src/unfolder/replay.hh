
#ifndef _C15U_REPLAY_HH_
#define _C15U_REPLAY_HH_


#include "stid/replay.hh"

#include "unfolder/pidmap.hh"
#include "unfolder/trail.hh"
#include "pes/cut.hh"

#include "verbosity.h"

namespace dpu {

class Replay : public stid::Replay
{
public :
   Replay (Unfolding &u_) : u (u_) {}

   /// Returns a replay sequence suitable for replaying the trail \p t.
   static Replay create (Unfolding &u, const Trail &t, unsigned limit = UINT_MAX)
   {
      Replay r(u);
      r.build_from (t, limit);
      return r;
   }

   /// Returns a replay sequence suitable setting Steroids in FREE mode
   static Replay create (Unfolding &u)
   {
      Replay r(u);
      r.finish ();
      return r;
   }

   /// Returns a replay sequence constructed from a vector of integers. The
   /// vector shall **not** include the last step {-1, -1}
   static Replay create (Unfolding &u, std::vector<int> &&v)
   {
      Replay r(u);
      ASSERT ((v.size() & 1) == 0);
      for (int i = 0; i < v.size(); i += 2) r.push_back ({v[i], v[i+1]});
      r.finish ();
      return r;
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

   /// Adds the {-1, -1} event to the end of the replay vector
   inline void finish ();

   /// Returns a string depicting the replay sequence; altidx is the offset at
   /// which the replay sequence switches between the trail and the alternative
   inline std::string str (unsigned altidx = UINT_MAX);

   // This object maps steroids tids to dpu pids when transforming the action
   // stream into events, and dpu pids to steroids tids when generating replays.
   // It is cleared before starting any of these two transformations, but stores
   // data shared across multiple methods involved in them. Pid #0 always maps
   // to tid t0, and vice versa, so the map always has at least one entry.
   Pidmap pidmap;
private :
   Unfolding &u;
};

// implementation of inline methods
#include "unfolder/replay.hpp"

} // namespace

#endif
