
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
//class State;

//=============================
class State
{

public:
   Machine & m;

   uint32_t & operator [] (unsigned) const; // return element at a specific position in tab
   State (Machine & m);
   State (const State &);
   State & operator = (const State & s);
   ~State ();
   int getNumProcs();
   std::vector<Process> & getSProcs();

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

   Machine (unsigned memsize, unsigned numprocs, unsigned numtrans = 0);

   bool        operator ==   (const Machine &) const;
   Process &   add_process   (unsigned numlocations);
   Trans   &   add_trans     (Process & p, unsigned src, unsigned dst);

   void sanity_check ();
   const State & init_state;
   std::vector<Trans> & getTrans();
   std::vector<Process> & getProcs();
   std::vector<Trans*>  getTrans1();



private :
   ir::State _init_state;
};

/*
 * ==============================
 */

class Process
{
public:
   int                               id;
   Machine &                         m;
   std::vector<Trans*>               trans;
   std::vector<std::vector<Trans *>> cfg;

   Process (Machine & m, unsigned numlocations);
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

   bool    enabled (const State &);
   State * fire    (const State &);

   Trans (Process & p, unsigned src, unsigned dst);
};

} // namespace ir

#endif
