
#include <cstring>
#include <cassert>
#include <algorithm>

#include "ir.hh"

namespace ir
{

Machine::Machine (unsigned memsize, unsigned numprocs, unsigned numtrans)
   : memsize (memsize)
   , init_state (_init_state)
   , _init_state (*this)
{
   if (numtrans == 0) numtrans = numprocs * 128;
   printf ("%p: Machine.ctor: procs.capacity %u trans.capacity %u memsize %u\n",
         this, numprocs, numtrans, memsize);
   
   assert (memsize >= numprocs);
   procs.reserve (numprocs);
   procs.shrink_to_fit ();
   trans.reserve (numtrans);
   trans.shrink_to_fit ();
}

bool Machine::operator == (const Machine & m) const
{
   return this == &m;
}

Process & Machine::add_process (unsigned numlocations)
{
   if (procs.size () == procs.capacity ())
      throw std::logic_error (
            "Tried to allocated more processes than the maximum permitted");
   procs.emplace_back (*this, numlocations);
   return procs.back ();
}

Trans & Machine::add_trans (Process & p, unsigned src, unsigned dst)
{
   Trans * t;

   // p is a process of this machine
   assert (&p.m == this);
   // src and dst make sense in that process
   assert (src < p.cfg.size ());
   assert (dst < p.cfg.size ());

   if (trans.size () == trans.capacity ())
      throw std::logic_error (
            "Tried to allocated more transitions than the maximum permitted");

   trans.emplace_back (p, src, dst);
   t = & trans.back ();
   p.trans.push_back (t);
   p.cfg[src].push_back (t);
   return *t;
}

Process::Process (Machine & m, unsigned numlocations)
   : m (m)
   , trans (0)
   , cfg (numlocations)
{
   cfg.shrink_to_fit ();
}

Trans::Trans (Process & p, unsigned src, unsigned dst)
   : src (src)
   , dst (dst)
   , proc (p)
{
   // src and dst make sense in that process
   assert (src < p.cfg.size ());
   assert (dst < p.cfg.size ());
   // this transition has been stored in p.trans
   assert (std::find (p.trans.begin(), p.trans.end(), this) != p.trans.end());
}


State::State (Machine & m)
   : m (m)
   , tab (new uint32_t[m.memsize])
{}
#if 0
{
   this.m = m;
   this.tab = new uint32_t[m.memsize];
   //this.tab = (m != NULL) ? new uint32_t[m.memsize] : NULL;
}
#endif

State::State (const State & s)
   : m (s.m)
   , tab (new uint32_t[s.m.memsize])
{
   memcpy (tab, s.tab, sizeof(uint32_t) * m.memsize);
}

State & State::operator= (const State & s)
{
   assert (&s.m == &m);
   memcpy (tab, s.tab, sizeof(uint32_t) * m.memsize);
   return *this;
}

State::~State ()
{
   delete[] tab;
}

uint32_t & State::operator [] (unsigned i)
{
   return tab[i];
}

} // namespace ir
