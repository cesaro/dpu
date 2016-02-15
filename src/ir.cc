
#include <cstring>
#include <cassert>
#include <algorithm>

#include "ir.hh"
#include "misc.hh"
#include "verbosity.h"

namespace ir
{

Machine::Machine (unsigned memsize, unsigned numprocs, unsigned numtrans)
   : memsize (memsize)
   , init_state (_init_state)
   , _init_state (*this)
{
   if (numtrans == 0) numtrans = numprocs * 128;
   DEBUG ("%p: Machine.ctor: memsize %u numprocs %u numtrans %u",
         this, memsize, numprocs, numtrans);
   
   assert (memsize >= numprocs);
   procs.reserve (numprocs);
   trans.reserve (numtrans);

   ASSERT (procs.capacity() == numprocs);
   ASSERT (trans.capacity() == numtrans);
}

Machine::~Machine ()
{
   DEBUG ("%p: Machine.dtor", this);
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

#ifdef CONFIG_DEBUG
   size_t i = procs.capacity ();
#endif
   procs.emplace_back (*this, numlocations, procs.size ());
   ASSERT (procs.capacity () == i);
   return procs.back();
}

std::string Machine::str () const
{
   std::string s;
   s += fmt ("Machine %p, %d processes, %d trans, %d variables:",
         this, procs.size (), trans.size (), memsize);
   for (auto & p : procs) s += "\n" + p.str ();
   return s;
}

void Machine::sanity_check ()
{
   // - the variables in all statements fit in the memory size
   // Trans.{offset,addr,localaddr} fit in memory
   // - the process id corresponds to the offset in Machine.procs
   // - Trans.{src,dst} are valid node numbers in the cfg
   // - match between Trans.code and infos in Trans.{addr,offset,localaddr}
   // - transitions of one thread do not overwrite its pc
}

// methods of class Process
Process::Process (Machine & m, unsigned numlocations, int id)
   : id (id)
   , m (m)
   , trans (0)
   , cfg (numlocations)

{
   cfg.shrink_to_fit ();
   DEBUG ("%p: Process.ctor: cfg.size %zd", this, cfg.size ());
}

Process::~Process ()
{
   DEBUG ("%p: Process.dtor", this);
}

Trans & Process::add_trans (unsigned src, unsigned dst)
{
   Trans * t;

   DEBUG ("%p: Process.add_trans: src %u dst %u cfg.size %zu",
         this, src, dst, cfg.size ());

   // src and dst make sense in that process
   ASSERT (src < cfg.size ());
   ASSERT (dst < cfg.size ());

   if (m.trans.size () == m.trans.capacity ())
      throw std::logic_error (
            "Tried to allocated more transitions than the maximum permitted");

   // add a new transition to the machine
#ifdef CONFIG_DEBUG
   size_t i = m.trans.capacity ();
#endif
   m.trans.emplace_back (*this, src, dst);
   ASSERT (m.trans.capacity () == i);

   // register it in this process
   t = & m.trans.back ();
   trans.push_back (t);
   cfg[src].push_back (t);
   return *t;
}

std::string Process::str () const
{
   std::string s;
   s += fmt ("Process %d (%p), %d trans, %d locations:\n",
         id, this, trans.size (), cfg.size ());
   for (unsigned src = 0; src < cfg.size (); ++src)
   {
      s += fmt (" src %2u:\n", src);
      for (auto t : cfg[src])
      {
         s += "  " + t->str() + "\n";
      }
   }
   return s;
}


//===methods for class Trans=========

Trans::Trans (Process & p, unsigned src, unsigned dst)
   : src (src)
   , dst (dst)
   , proc (p)
{
   DEBUG ("%p: Trans.ctor: src %u dst %u p.id %d",
         this, src, dst, p.id);
   // src and dst make sense in that process
   assert (src < p.cfg.size ());
   assert (dst < p.cfg.size ());
   // this transition has been stored in p.trans
   assert (std::find (p.trans.begin(), p.trans.end(), this) == p.trans.end());
}

Trans::~Trans ()
{
   DEBUG ("%p: Trans.dtor", this);
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
	DEBUG("fire");
	State s1(s);
	s1[this->proc.id] ++;
	return new State (s1);
}


std::string Trans::str () const
{
   return fmt ("%p proc.id %d src %u dst %u type '%s' code '%s'",
            this, this->proc.id, this->src, this->dst,
            this->type_str(), this->code.str().c_str());
}

const char * Trans::type_str () const
{
   switch (type)
   {
   case RD  : return "RD";
   case WR  : return "WR";
   case LOC : return "LOC";
   case SYN : return "SYN";
   }
   return 0;
}

//===methods for class State==========

State::State (Machine & m)
   : m (m)
   , tab (new uint32_t[m.memsize])
{
   for (unsigned i = 0; i < m.memsize; i++) tab[i] = 0;
}

State::State (const State & other)
   : m (other.m)
   , tab (new uint32_t[other.m.memsize])
{
   memcpy (tab, other.tab, sizeof(uint32_t) * m.memsize);
}

State::State (State && other)
   : m (other.m)
   , tab (other.tab)
{
   other.tab = 0;
}

State & State::operator= (const State & other)
{
   ASSERT (other.m == m);
   memcpy (tab, other.tab, sizeof(uint32_t) * m.memsize);
   return *this;
}

State & State::operator= (State && other)
{
   ASSERT (other.m == m);
   tab = other.tab;
   other.tab = 0;
   return *this;
}

State::~State ()
{
   delete[] tab;
}

const uint32_t & State::operator [] (unsigned i) const
{
   return tab[i];
}

uint32_t & State::operator [] (unsigned i)
{
   return tab[i];
}

} // namespace ir
