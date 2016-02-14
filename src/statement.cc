
#include <cstdio>

#include "misc.hh"
#include "verbosity.h"
#include "statement.hh"

namespace ir
{

Var * Var::make (unsigned tab, Expr * idx)
{
   return new Var (tab, idx);
}

Var * Var::clone () const
{
   return new Var (*this);
}

Var::Var (unsigned tab, Expr * idx)
   : var (tab)
   , idx (idx)
{
   DEBUG ("Var.ctor: this %p %s var %d idx %p '%s'",
         this, type () == VAR ? "VAR" : "ARRAY",
         var, idx, idx ? idx->str().c_str() : "");
}

Var::Var (const Var & other)
   : var (other.var)
   , idx (other.idx ? other.idx->clone () : 0)
{
   DEBUG ("Var.ctor: this %p other %p '%s' (copy)",
         this, &other, other.str().c_str());
}

#if 0
Var::Var (Var && other)
   : var (other.var)
   , idx (other.idx) // steal the pointer !
{
   DEBUG ("Var.ctor: this %p other %p '%s' (move)",
         this, &other, other.str().c_str());
   other.idx = 0;
}
#endif

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

Expr * Expr::make (int32_t imm)
{
   Expr * e = new Expr ();
   e->type = IMM;
   e->imm = imm;
   DEBUG ("Expr.make: this %p IMM %d", e, imm);
   return e;
}

Expr * Expr::make (Var * v)
{
   Expr * e = new Expr ();
   e->type = VAR;
   e->v = v;
   DEBUG ("Expr.make: this %p VAR %p '%s'",
         e, e->v, e->v->str().c_str());
   return e;
}

Expr * Expr::make (op_t o, Expr * e1)
{
   Expr * e = new Expr ();
   e->type = OP1;
   e->op = o;
   e->expr1 = e1;
   e->expr2 = 0;
   DEBUG ("Expr.make: this %p op %s e1 %p '%s'",
         e, e->op_str (), e1, e1->str().c_str());
   ASSERT (e->op_arity () == 1);
   return e;
}

Expr * Expr::make (op_t o, Expr * e1, Expr * e2)
{
   Expr * e = new Expr ();
   e->type = OP2;
   e->op = o;
   e->expr1 = e1;
   e->expr2 = e2;
   DEBUG ("Expr.make: this %p op %s e1 %p '%s' e2 %p '%s'",
         e, e->op_str (),
         e1, e1->str().c_str(),
         e2, e2->str().c_str());
   ASSERT (e->op_arity () == 2);
   return e;
}

Expr * Expr::clone () const
{
   return new Expr (*this);
}

Expr::Expr ()
   : type (IMM)
   , imm (0)
{
   DEBUG ("Expr.ctor: this %p", this);
}

Expr::Expr (const Expr & other)
   : type (other.type)
{
   DEBUG ("Expr.ctor: this %p other %p '%s' (copy)",
         this, &other, other.str().c_str());
   switch (type)
   {
   case VAR :
      v = other.v->clone ();
      break;
   case IMM :
      imm = other.imm;
      break;
   case OP1 :
      op = other.op;
      expr1 = other.expr1->clone ();
      expr2 = 0;
      break;
   case OP2 :
      op = other.op;
      expr1 = other.expr1->clone ();
      expr2 = other.expr2->clone ();
      break;
   }
}

#if 0
Expr::Expr (Expr && other)
   : type (other.type)
{
   DEBUG ("Expr.ctor: this %p other %p '%s' (move)",
         this, &other, other.str().c_str());
   steal (other);
}
#endif

Expr & Expr::operator = (const Expr & other)
{
   DEBUG ("Expr.op= this %p other %p '%s' (copy)",
         this, &other, other.str().c_str());
   Expr temp (other);
   steal (temp);
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
      break;
   case IMM :
      imm = from.imm;
      break;
   case OP1 :
   case OP2 :
      op = from.op;
      expr1 = from.expr1;
      expr2 = from.expr2;
      break;
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
