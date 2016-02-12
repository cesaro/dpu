
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
   procs.reserve (2 * numprocs);
   procs.shrink_to_fit ();
   trans.reserve (2 * numtrans);
   trans.shrink_to_fit ();
}

bool Machine::operator == (const Machine & m) const
{
   return this == &m;
}

#if 0
Machine &  Machine:: operator =    (const Machine & m)
{
  // this->init_state = m.init_state;
   this->memsize     = m.memsize;
   for (int i = 0; i < m.procs.size(); i++)
      this->procs[i] = m.procs[i];
   for (int i = 0; i < m.trans.size(); i++)
      this->trans[i] = m.trans[i];

   return *this;
}
#endif

#if 0
Process & Machine::add_process (unsigned numlocations, int id)
{
   if (procs.size () > procs.capacity ())
      throw std::logic_error (
            "Tried to allocated more processes than the maximum permitted");
   procs.emplace_back (*this, numlocations, id);
   return procs.back();
}

Trans & Machine::add_trans (Process & p, unsigned src, unsigned dst)
{
   Trans * t;

  // printf ("Machine.add_trans: p %p src %u dst %u p.cfg.size %zu\n", &p, src, dst, p.cfg.size ());

   // p is a process of this machine
   assert (&p.m == this);
   // src and dst make sense in that process
   assert (src < p.cfg.size ());
   assert (dst < p.cfg.size ());

   if (trans.size () > trans.capacity ())
      throw std::logic_error (
            "Tried to allocated more transitions than the maximum permitted");

   trans.emplace_back (p, src, dst);
   t = & trans.back ();
   p.trans.push_back (t);
   p.cfg[src].push_back (t);
   printf("in add trans: t.src: %d and t.dest: %d, t.proc.id: %d \n", t->src, t->dst, t->proc.id);
   return *t;
}

#endif

void Machine::add_process (unsigned numlocations, int id)
{
   if (procs.size () > procs.capacity ())
      throw std::logic_error (
            "Tried to allocated more processes than the maximum permitted");
   procs.emplace_back (*this, numlocations, id);
}

void Machine::add_trans (Process & p, unsigned src, unsigned dst)
{
   Trans * t;

  // printf ("Machine.add_trans: p %p src %u dst %u p.cfg.size %zu\n", &p, src, dst, p.cfg.size ());

   // p is a process of this machine
   assert (&p.m == this);
   // src and dst make sense in that process
   assert (src < p.cfg.size ());
   assert (dst < p.cfg.size ());

   if (trans.size () > trans.capacity ())
      throw std::logic_error (
            "Tried to allocated more transitions than the maximum permitted");

   trans.emplace_back (p, src, dst);
   t = & trans.back ();
   p.trans.push_back (t);
   p.cfg[src].push_back (t);
   printf("in add trans: t.src: %d and t.dest: %d, t.proc.id: %d \n", t->src, t->dst, t->proc.id);

}


std::vector<Trans> & Machine::getTrans()
{
   return trans;
}

std::vector<Process> & Machine::getProcs()
{
	return procs;

}

// methods of class Process
Process::Process (Machine & m, unsigned numlocations, int id)
   : id (id)
   , m (m)
   , trans (0)
   , cfg (numlocations)

{
   cfg.shrink_to_fit ();
 //  printf ("Process.ctor: this %p cfg.size %zd\n", this, cfg.size ());
}
//===methods for class Trans=========

Trans::Trans (Process & p, unsigned src, unsigned dst)
   : src (src)
   , dst (dst)
   , proc (p)
{
   // src and dst make sense in that process
   assert (src < p.cfg.size ());
   assert (dst < p.cfg.size ());
   // this transition has been stored in p.trans, if already, no insert
   assert (std::find (p.trans.begin(), p.trans.end(), this) == p.trans.end());
   printf("in constructor; proc.id: %d", proc.id);
}

bool Trans::enabled (const State & s)
{
	// for temporarily compute enable set
   if (src == s[proc.id])
      return true;
   return false;
}

State * Trans::fire (const State &s)
{
	printf("fire \n");
	State s1(s);
	s1[this->proc.id] ++;
	return new State (s1);
}

//===methods for class State==========

State::State (Machine & m)
   : m (m)
   , tab (new uint32_t[m.memsize])
{
   for (unsigned int i = 0; i < m.memsize; i++)
	   *(tab + i) = 0;
}
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

uint32_t & State::operator [] (unsigned i) const
{
   return tab[i];
}

int State::getNumProcs()
{
   return m.procs.size();
}

std::vector<Process> & State::getSProcs()
{
	return m.getProcs();
}

} // namespace ir
