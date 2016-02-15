
#ifndef __IR_HH_
#define __IR_HH_

#include <cstdint>
#include <iostream>
#include <array>
#include <vector>

#include "codeblock.hh"

namespace ir
{

class Trans;
class Process;
class Machine;

//=============================
class State
{

public:
   Machine & m;

   State (Machine & m);
   State (const State & other);
   State (State && other);
   State & operator = (const State & other);
   State & operator = (State && other);
   ~State ();

   const uint32_t & operator [] (unsigned idx) const; // access the tab
   uint32_t & operator [] (unsigned idx); // access the tab

private:
   uint32_t * tab;  // as an array of uint32_t
};
//==============================

class Machine
{
public:
   std::vector<Process> procs;
   std::vector<Trans>   trans;
   unsigned             memsize; // number of variable

   Machine  (unsigned memsize, unsigned numprocs, unsigned numtrans = 0);
   Machine  (const Machine & other) = delete;
   Machine  (Machine && other)      = delete;
   ~Machine ();

   bool        operator ==   (const Machine &) const;
   Machine &   operator =    (const Machine & m) = delete;
   Machine &   operator =    (Machine && m)      = delete;

   Process &   add_process   (unsigned numlocations);
   std::string str           () const;
   void        sanity_check  ();

   const State & init_state;

private :
   ir::State _init_state;
};

/*
 * ==============================
 */

class Process
{
public:
   const int                         id;
   Machine &                         m;
   std::vector<Trans*>               trans;
   std::vector<std::vector<Trans *>> cfg;

   Process  (Machine & m, unsigned numlocations, int id);
   ~Process ();

   Trans &      add_trans     (unsigned src, unsigned dst);
   std::string  str           () const;
private :
};

//================================
class Trans
{
public:
   enum {RD, WR, LOC, SYN} type;

   unsigned              src;
   unsigned              dst;
   Process &             proc;
   unsigned              addr;
   unsigned              offset;
   std::vector<unsigned> localaddr;
   Codeblock             code;

   Trans (Process & p, unsigned src, unsigned dst);
   ~Trans ();

   std::string  str      () const;
   const char * type_str () const;
   bool         enabled  (const State &);
   State *      fire     (const State &);
};

} // namespace ir

#endif
