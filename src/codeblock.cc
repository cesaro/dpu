
#include "misc.hh"
#include "codeblock.hh"

namespace ir
{

Codeblock::Codeblock ()
{
}

Codeblock::~Codeblock ()
{
}

std::string Codeblock::str () const
{
   //return fmt ("%p stm %p '%s'", this, &this->stm, this->stm.str().c_str());
   return this->stm.str();
}

} // namespace ir
