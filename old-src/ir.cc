
#include <cstring>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <ctime>


#include "ir.hh"
#include "misc.hh"
#include "verbosity.h"

namespace ir
{

Machine::Machine (unsigned memsize, unsigned numprocs, unsigned numtrans) :
   memsize (memsize),
   init_state (_init_state),
   _init_state (*this)
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
   // - match between Trans.code and infos in Trans.{type,addr,offset,localaddr}
   // - transitions of one thread do not overwrite the pcs
   // - the program is deterministic
}

void Machine:: change_init_state(std::vector<uint32_t> t)
{
   init_state.change_tab(t);
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


// === methods for class Trans =========

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

bool Trans::enabled (const State & s) const
{
   if (src != s[proc.id]) return false;
   return s.enabled (code);
}

void Trans::fire (State & s) const
{
   ASSERT (enabled (s));
	s[proc.id] = dst;
   s.execute (code);
}

std::string Trans::str () const
{
   return fmt ("%p:  proc.id %d src %u dst %u type '%s' code '%s'",
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
#if 0
bool Trans::operator == (const Trans & t) const
{
   if (proc.id == t.proc.id) && (code == t.code) && (src  == t.src) && (dst  == t.dst)
         && (var  == t.var) && (localvars == t.localvars) && (type == t.type) && (offset == t.offset)
   return true;
}
#endif
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

uint32_t & State::operator [] (const Var * v)
{
   return eval (v);
}

void State::enabled (std::vector<Trans*> & list) const
{
   list.clear ();

   // we assume that the program is data-deterministic
   for (unsigned pid = 0; pid < m.procs.size(); ++pid)
   {
      //the current pc for thread pid is a valid location of the CFG
      ASSERT (tab[pid] < m.procs[pid].cfg.size());

      // for each transition of process pid whose src field is tab[pid]
      for (auto t : m.procs[pid].cfg[tab[pid]])
      {
         if (t->enabled (*this)) list.push_back (t);
      }
   }
}


bool State::enabled (const Codeblock & code) const
{
   return enabled (code.stm);
}

bool State::enabled (const Stm & stm) const
{
   switch (stm.type)
   {
   case Stm::ASGN :
   case Stm::ERROR :
   case Stm::EXIT :
      return true;
   case Stm::LOCK :
      // a lock statement is enabled if the lock is not taken (evaluates to 0)
      return not eval (stm.lhs);
   case Stm::UNLOCK :
      // similarly, unlock enabled iff the lock is released (evaluates to != 0)
      return eval (stm.lhs);
   case Stm::ASSUME :
      // assume enabled if expression evaluates to non-zero value
      return eval (stm.expr);
   }
   return false; // unreachable code
}

void State::execute (const Codeblock & code)
{
   execute (code.stm);
}

void State::execute (const Stm & stm)
{
   ASSERT (enabled (stm));
   switch (stm.type)
   {
   case Stm::ASGN :
      // evaluate the expression and assign it to the left-hand-side variable
      eval (stm.lhs) = eval (stm.expr);
      break;
   case Stm::LOCK :
      // the lock should not be taken; set it to 1 (taken)
      eval (stm.lhs) = 1;
      break;
   case Stm::UNLOCK :
      // the lock should be taken (different than zero); set lock to 0 (not taken)
      eval (stm.lhs) = 0;
      break;
   case Stm::ASSUME :
   case Stm::ERROR :
   case Stm::EXIT :
      // nothing to do for ASSUME, ERROR, or EXIT
      break;
   }
}

uint32_t State::eval (const Expr * e) const
{
   switch (e->type)
   {
   case Expr::VAR : return eval (e->v);
   case Expr::IMM : return (uint32_t) e->imm;
   case Expr::OP1 :
   case Expr::OP2 :
      switch (e->op)
      {
      case Expr::ADD : return eval (e->expr1) + eval (e->expr2);
      case Expr::SUB : return eval (e->expr1) - eval (e->expr2);
      case Expr::MUL : return eval (e->expr1) * eval (e->expr2);
      case Expr::DIV : return eval (e->expr1) / eval (e->expr2);
      case Expr::MOD : return eval (e->expr1) % eval (e->expr2);
      case Expr::EQ :  return eval (e->expr1) == eval (e->expr2);
      case Expr::NE :  return eval (e->expr1) != eval (e->expr2);
      case Expr::LE :  return eval (e->expr1) <= eval (e->expr2);
      case Expr::LT :  return eval (e->expr1) < eval (e->expr2);
      case Expr::AND : return eval (e->expr1) and eval (e->expr2);
      case Expr::OR :  return eval (e->expr1) or eval (e->expr2);
      case Expr::NOT : return not eval (e->expr1);
      }
   }
   return 0; // unreachable code, just to avoid copmiler warning
}

uint32_t & State::eval (const Var * v) const
{
   switch (v->type ())
   {
   case Var::VAR :
      ASSERT (v->var < m.memsize);
      return tab[v->var];
   case Var::ARRAY :
      unsigned idx = v->var + eval (v->idx);
      if (idx >= m.memsize)
      {
         throw std::range_error (fmt
               ("Out of bounds: expression '%s' tries to access address %u",
               v->str().c_str(), idx));
      }
      return tab[idx];
   }
   return tab[0]; // unreachable code, just to avoid compiler warning
}

std::string State::str () const
{
   std::string s;
   unsigned i = 0;

   for ( ; i < m.procs.size (); ++i) s += fmt ("%-3u ", tab[i]);
   s += "  ";
   for ( ; i < m.memsize; ++i) s += fmt ("%04u ", tab[i]);
   return s;
}

std::string State::str_header () const
{
   std::string s;
   unsigned i = 0;

   for ( ; i < m.procs.size (); ++i) s += fmt ("p%-2u ", i);
   s += "  ";
   for ( ; i < m.memsize; ++i) s += fmt ("v%-3u ", i);
   return s;
}

void  State::change_tab  (std::vector<uint32_t> t) const
{
   for (unsigned i = 0; i < t.size(); i++)
      tab[i] = t[i];
}

void simulate (Machine * m)
{
    DEBUG ("BEGIN simulation");
    DEBUG ("%s", m->str().c_str());

    ir::State s (m->init_state);
    std::vector<ir::Trans*> ena;
    unsigned seed, i;

    seed = std::time (0); // use current time as seed to get random numbers
    //seed = 1234;
    DEBUG ("Using seed %u", seed);
    std::srand (seed);

    while (true)
    {
       DEBUG ("========================================");
       DEBUG ("%s", s.str_header().c_str());
       DEBUG ("%s", s.str().c_str());
       s.enabled (ena);
       DEBUG ("enables %d transitions:", ena.size ());
       for (auto t : ena) DEBUG (" %s", t->str().c_str());
       if (ena.size () == 0) break;

       i = std::rand() % ena.size ();
       DEBUG ("firing idx %u, proc %d", i, ena[i]->proc.id);
       ena[i]->fire (s);
    }
    DEBUG ("END simulation");
}

} // namespace ir
