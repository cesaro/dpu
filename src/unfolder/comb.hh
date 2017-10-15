#ifndef _C15U_COMB_HH_
#define _C15U_COMB_HH_

#include <vector>
#include <string>

#include "pes/event.hh"
#include "unfolder/c15unfolder.hh"

#include "misc.hh"
#include "verbosity.h"

#define COMB_RESERVED_SIZE 32
#define COMB_RESERVED_SPIKE_SIZE 2046

namespace dpu
{

class Spike : public std::vector<Event *>
{
public:
   Spike (unsigned cap) :
      std::vector<Event*> ()
   {
      reserve (cap);
      DEBUG ("c15u: comb: spike: ctor: cap %u", cap);
   }
};

class Comb
{
public:
   typedef Spike* iterator;
   typedef const Spike* const_iterator;
   typedef Spike& reference;
   typedef const Spike& const_reference;

   Comb (Altalgo a, unsigned kbound) :
      spikes (),
      _size (0)
   {
      unsigned cap;
      switch (a) {
      case Altalgo::SDPOR :
      case Altalgo::ONLYLAST :
         cap = 1;
         break;
      case Altalgo::KPARTIAL :
         cap = kbound;
         break;
      case Altalgo::OPTIMAL :
         cap = COMB_RESERVED_SIZE;
         break;
      }
      reserve (cap);
   }

   void clear () { _size = 0; }
   unsigned size () const { return _size; }
   bool empty () const { return _size == 0; }
   unsigned capacity () const { return spikes.size(); }

   // operator []
   reference operator[] (unsigned idx) { return spikes[idx]; }
   const_reference operator[] (unsigned idx) const { return spikes[idx]; }

   // iterators
   iterator begin() { return spikes.data() + 0; }
   const_iterator begin() const { return spikes.data() + 0; }
   iterator end() { return spikes.data() + _size; }
   const_iterator end() const { return spikes.data() + _size; }

   void reserve (unsigned cap)
   {
      unsigned i;
      unsigned oldcap;
      oldcap = capacity();
      DEBUG ("c15u: comb: reserve: oldcap %u newcap %u", oldcap, cap);
      spikes.reserve (cap);
      for (i = oldcap; i < cap; i++)
      {
         spikes.emplace_back (COMB_RESERVED_SIZE);
      }
   }

   // adding new spikes
   void add_spike (const Event *e)
   {
      unsigned idx;

      ASSERT (_size <= capacity());
      idx = _size;
      if (_size == capacity()) reserve (_size * 2);
      _size++;
      spikes[idx].clear(); // we clear spikes lazily
      e->icfls (spikes[idx]);
   }

   std::string str ()
   {
      std::string s;
      unsigned i, j;

      for (i = 0; i < _size; i++)
      {
         s += fmt ("spike %u len %u [", i, spikes[i].size());
         for (j = 0; j < spikes[i].size(); j++)
         {
            s += fmt ("%s ", spikes[i][j]->suid().c_str());
         }
         if (j) s.pop_back();
         s += "]\n";
      }
      return s;
   }

private:
   std::vector<Spike> spikes;
   unsigned _size;
};

} // end of namespace
#endif

