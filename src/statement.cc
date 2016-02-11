
#include <cstdio>

#include "misc.hh"
#include "verbosity.h"
#include "statement.hh"

namespace ir
{

Var::Var (unsigned v)
   : var (v)
   , idx (0)
{
   DEBUG ("Var.ctor: this %p VAR var %d", this, v);
}

Var::Var (unsigned tab, Expr & idx)
   : var (tab)
   , idx (new Expr (idx))
{
   DEBUG ("Var.ctor: this %p ARRAY var %d idx %p '%s'",
         this, var, this->idx, this->idx->str().c_str());
}

Var::Var (const Var & other)
   : var (other.var)
   , idx (other.idx ? new Expr (*other.idx) : 0)
{
   DEBUG ("Var.ctor: this %p other %p '%s' (copy)",
         this, &other, other.str().c_str());
}

Var::Var (Var && other)
   : var (other.var)
   , idx (other.idx) // steal the pointer !
{
   DEBUG ("Var.ctor: this %p other %p '%s' (move)",
         this, &other, other.str().c_str());
   other.idx = 0;
}

Var & Var::operator = (const Var & other)
{
   DEBUG ("Var.op= this %p other %p '%s' (copy)", this, &other, other.str().c_str());
   Var temp (other);
   steal (temp);
   return *this;
}

Var & Var::operator = (Var && other)
{
   DEBUG ("Var.op= this %p other %p '%s' (move)", this, &other, other.str().c_str());
   steal (other);
   return *this;
}

Var::~Var ()
{
   DEBUG ("Var.dtor: this %p", this);
   delete idx;
}

Var::type_t Var::type () const
{
   return idx ? ARRAY : VAR;
}

std::string Var::str () const
{
   if (idx == NULL) return fmt ("v%u", var);
   return fmt ("v%u[%s]", var, idx->str().c_str());
}

void Var::steal (Var & from)
{
   delete idx;
   var = from.var;
   idx = from.idx;
   from.idx = 0;
}

Expr::Expr (int32_t imm)
   : type (IMM)
   , imm (imm)
{
   DEBUG ("Expr.ctor: this %p IMM %d", this, imm);
}

Expr::Expr (Var & v)
   : type (VAR)
   , v (new Var (v))
{
   DEBUG ("Expr.ctor: this %p VAR %p '%s'",
         this, this->v, this->v->str().c_str());
}

Expr::Expr (op_t o, Expr & e1)
   : type (OP1)
   , op (o)
   , expr1 (new Expr (e1))
   , expr2 (0)
{
   DEBUG ("Expr.ctor: this %p op %s e1 '%s'",
         this, op_str (), e1.str().c_str());

   ASSERT (op_arity () == 1);
}

Expr::Expr (op_t o, Expr & e1, Expr & e2)
   : type (OP2)
   , op (o)
   , expr1 (new Expr (e1))
   , expr2 (new Expr (e2))
{
   DEBUG ("Expr.ctor: this %p op %s e1 '%s' e2 '%s'",
         this, op_str (), e1.str().c_str(), e2.str().c_str());

   ASSERT (op_arity () == 2);
}

Expr::Expr (const Expr & other)
   : type (other.type)
{
   DEBUG ("Expr.ctor: this %p other %p '%s' (copy)",
         this, &other, other.str().c_str());
   switch (type)
   {
   case VAR :
      v = new Var (*other.v);
      break;
   case IMM :
      imm = other.imm;
      break;
   case OP1 :
      op = other.op;
      expr1 = new Expr (*other.expr1);
      expr2 = 0;
      break;
   case OP2 :
      op = other.op;
      expr1 = new Expr (*other.expr1);
      expr2 = new Expr (*other.expr2);
      break;
   default :
      ASSERT (0);
   }
}

Expr::Expr (Expr && other)
   : type (other.type)
{
   DEBUG ("Expr.ctor: this %p other %p '%s' (move)",
         this, &other, other.str().c_str());
   steal (other);
}

Expr & Expr::operator = (Expr other)
{
   DEBUG ("Expr.op= this %p other %p '%s' (copy)",
         this, &other, other.str().c_str());
   steal (other);
   return *this;
}

Expr & Expr::operator = (Expr && other)
{
   DEBUG ("Expr.op= this %p other %p '%s' (move)",
         this, &other, other.str().c_str());
   steal (other);
   return *this;
}

Expr::~Expr ()
{
   DEBUG ("Expr.dtor: this %p type %s", this, type_str ());
   switch (type)
   {
   case VAR :
      delete v;
      break;
   case IMM :
      break;
   case OP1 :
   case OP2 :
      delete expr1;
      delete expr2;
   }
}

void Expr::steal (Expr & from)
{
   switch (type)
   {
   case VAR :
      delete v;
      break;
   case OP1:
   case OP2:
      delete expr1;
      delete expr2;
   case IMM:
      break;
   }
   
   type = from.type;
   switch (type)
   {
   case VAR :
      v = from.v;
   case IMM :
      imm = from.imm;
   case OP1 :
   case OP2 :
      op = from.op;
      expr1 = from.expr1;
      expr2 = from.expr2;
   }

   // prevents from deleting the 'moved' pointers
   from.type = IMM;
}

const char * Expr::type_str () const
{
   switch (type)
   {
   case VAR : return "VAR";
   case IMM : return "IMM";
   case OP1 : return "OP1";
   case OP2 : return "OP2";
   }
   return 0;
}

int Expr::op_arity () const
{
   switch (type)
   {
   case VAR :
   case IMM : return 0;
   case OP1 : return 1;
   case OP2 : return 2;
   }
   return 0;
}

const char * Expr::op_str () const
{
   switch (op)
   {
   case ADD : return "+";
   case SUB : return "-";
   case DIV : return "/";
   case EQ  : return "==";
   case LE  : return "<=";
   case LT  : return "<";
   case AND : return "and";
   case OR  : return "or";
   case NOT : return "not";
   }
   return 0;
}

std::string Expr::str () const
{
   switch (type)
   {
   case VAR :
      return v->str ();
   case IMM :
      return fmt ("%d", imm);
   case OP1 :
      return fmt ("%s (%s)", op_str(), expr1->str().c_str());
   case OP2 :
      return fmt ("(%s) %s (%s)",
            op_str(), expr1->str().c_str(), expr2->str().c_str());
   }
   ASSERT (0);
}


} // namespace ir
