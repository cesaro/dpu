
#ifndef _PROBDIST_HH_
#define _PROBDIST_HH_

#include <map>
#include <vector>
#include <string>
#include <sstream>

template <class T>
class Probdist
{
public :
   Probdist ();

   /// Read-only access to the sample counts
   const std::map<T,unsigned> &counters;

   /// Resets the state (0 samples acquired)
   void clear ();

   /// Updates the mass map to account for calls to sample() after the last
   /// call to finalize()
   void finalize ();

   /// Increments by 1 the number of times event e has occurred in the acquired
   /// sample
   void sample (T e);

   /// Returns the number of times that sample() has been called
   size_t size () const;

   /// Returns the number of times that an event equal to e happened
   unsigned count (T e) const;

   /// Returns the probability mass of e in the acquired sample
   float mass (T e) const;

   // Assuming that T is numeric (int, float, etc), these return the minimum,
   // maximum, and average in the sample
   T max () const;
   T min () const;
   float average () const;

   /// size/nc
   std::string summary_snc () const;
   /// min/max/avg
   std::string summary_mma () const;
   /// nc/min/max/avg
   std::string summary_mmac () const;
   /// {ev:count/mass}
   std::string summary_freq_maxc (unsigned limit) const;

protected :
   /// 0:129/0.2%; 18:99323/81.4%; 280:12/0.5%
   void summary_freq (std::ostringstream &ss, const std::vector<std::pair<T,unsigned>> &v) const;
   /// 0=129/0.2%
   void summary_freq (std::ostringstream &ss, const T &e) const;
   /// Returns the probability mass of the given count
   float count2mass (unsigned c) const;
   
   size_t samples;
   std::map<T,unsigned> _counters;
};

// implementation
#include "probdist.tcc"

#endif
