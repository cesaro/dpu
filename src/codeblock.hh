
#ifndef __CODEBLOCK_HH_
#define __CODEBLOCK_HH_

#include "statement.hh"

namespace ir
{

class Codeblock
{
public:
   Codeblock ();
   ~Codeblock ();

private:
   Stm stm;
};

}

#endif
