
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
   DEBUG ("%p: Var.ctor: type %s var %d idx %p '%s'",
         this, type () == VAR ? "VAR" : "ARRAY",
         var, idx, idx ? idx->str().c_str() : "");
}

Var::Var (const Var & other)
   : var (other.var)
   , idx (other.idx ? other.idx->clone () : 0)
{
   DEBUG ("%p: Var.ctor: other %p '%s' (copy)",
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
   DEBUG ("%p: Var.op= other %p '%s' (copy)", this, &other, other.str().c_str());
   Var temp (other);
   steal (temp);
   return *this;
}

Var & Var::operator = (Var && other)
{
   DEBUG ("%p: Var.op= other %p '%s' (move)", this, &other, other.str().c_str());
   steal (other);
   return *this;
}

Var::~Var ()
{
   DEBUG ("%p: Var.dtor", this);
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
   DEBUG ("%p: Expr.ctor: IMM %d", e, imm);
   return e;
}

Expr * Expr::make (Var * v)
{
   Expr * e = new Expr ();
   e->type = VAR;
   e->v = v;
   DEBUG ("%p: Expr.ctor: VAR %p '%s'",
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
   DEBUG ("%p: Expr.ctor: op '%s' e1 %p '%s'",
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
   DEBUG ("%p: Expr.ctor: op '%s' e1 %p '%s' e2 %p '%s'",
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
   //DEBUG ("Expr.ctor: this %p", this);
}

Expr::Expr (const Expr & other)
   : type (other.type)
{
   DEBUG ("%p: Expr.ctor: other %p '%s' (copy)",
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
   DEBUG ("%p: Expr.ctor: other %p '%s' (move)",
         this, &other, other.str().c_str());
   steal (other);
}
#endif

Expr & Expr::operator = (const Expr & other)
{
   DEBUG ("%p: Expr.op= other %p '%s' (copy)",
         this, &other, other.str().c_str());
   Expr temp (other);
   steal (temp);
   return *this;
}

Expr & Expr::operator = (Expr && other)
{
   DEBUG ("%p: Expr.op= other %p '%s' (move)",
         this, &other, other.str().c_str());
   steal (other);
   return *this;
}

Expr::~Expr ()
{
   DEBUG ("%p: Expr.dtor: type %s", this, type_str ());
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
   return 0; // unreachable code
}

int Expr::op_arity () const
{
   switch (op)
   {
   case ADD :
   case SUB :
   case MUL :
   case DIV :
   case MOD :
   case EQ  :
   case NE  :
   case LE  :
   case LT  :
   case AND :
   case OR  : return 2;
   case NOT : return 1;
   }
   return -1; // unreachable code
}

const char * Expr::op_str () const
{
   switch (op)
   {
   case ADD : return "+";
   case SUB : return "-";
   case MUL : return "*";
   case DIV : return "/";
   case MOD : return "%";
   case EQ  : return "==";
   case NE  : return "!=";
   case LE  : return "<=";
   case LT  : return "<";
   case AND : return "and";
   case OR  : return "or";
   case NOT : return "not";
   }
   return 0; // unreachable code
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
            expr1->str().c_str(), op_str(), expr2->str().c_str());
   }
   ASSERT (0);
}

Stm::Stm (type_t t, Expr * e)
   : type (t)
   , lhs (0)
   , expr (e)
{
   DEBUG ("%p: Stm.ctor: type %s lhs (null) '' expr %p '%s'",
         this, type_str (), e, e ? e->str().c_str() : "");

#ifdef CONFIG_DEBUG
   switch (t)
   {
   case ASGN :
   case LOCK :
   case UNLOCK :
      ASSERT (0); break;
   case ASSUME :
      ASSERT (e); break;
   case ERROR :
   case EXIT :
      ASSERT (not e); break;
   }
#endif
}

Stm::Stm (type_t t, Var * v, Expr * e)
   : type (t)
   , lhs (v)
   , expr (e)
{
   ASSERT (v);
   DEBUG ("%p: Stm.ctor: type %s lhs %p '%s' expr %p '%s'",
         this, type_str (),
         v, v->str().c_str(),
         e, e ? e->str().c_str() : "");

#ifdef CONFIG_DEBUG
   switch (t)
   {
   case ASGN :
      ASSERT (e); break;
   case ASSUME :
   case ERROR :
   case EXIT :
      ASSERT (0); break;
   case LOCK :
   case UNLOCK :
      ASSERT (not e);
      break;
   }
#endif
}

Stm::Stm (const Stm & other)
   : type (other.type)
   , lhs (other.lhs->clone ())
   , expr (other.expr->clone ())
{
   DEBUG ("%p: Stm.ctor: other %p '%s' (copy)",
         this, &other, other.str().c_str());
}

Stm::Stm (Stm && other)
   : type (other.type)
   , lhs (other.lhs)
   , expr (other.expr)
{
   DEBUG ("%p: Stm.ctor: other %p '%s' (move)",
         this, &other, other.str().c_str());

   other.type = EXIT;
   other.lhs = 0;
   other.expr = 0;
}

Stm & Stm::operator = (const Stm & other)
{
   DEBUG ("%p: Stm.op=: other %p '%s' (copy)",
         this, &other, other.str().c_str());
   Stm tmp (other);
   steal (tmp);
   return *this;
}

Stm & Stm::operator = (Stm && other)
{
   DEBUG ("%p: Stm.op=: other %p '%s' (move)",
         this, &other, other.str().c_str());
   steal (other);
   return *this;
}

Stm::~Stm ()
{
   delete lhs;
   delete expr;
}

std::string Stm::str () const
{
   switch (type)
   {
   case ASGN   : return fmt ("%s = %s", lhs->str().c_str(), expr->str().c_str());
   case ASSUME : return fmt ("assume (%s)", expr->str().c_str());
   case LOCK   : return fmt ("lock (%s)", lhs->str().c_str());
   case UNLOCK : return fmt ("unlock (%s)", lhs->str().c_str());
   case ERROR  : return std::string ("error");
   case EXIT   : return std::string ("exit");
   }
   return 0;
}

const char * Stm::type_str () const
{
   switch (type)
   {
   case ASGN   : return "ASGN";
   case ASSUME : return "ASSUME";
   case LOCK   : return "LOCK";
   case UNLOCK : return "UNLOCK";
   case ERROR  : return "ERROR";
   case EXIT   : return "EXIT";
   }
   return 0;
}

void Stm::steal (Stm & from)
{
   // clear
   delete lhs;
   delete expr;

   // steal
   type = from.type;
   lhs = from.lhs;
   expr = from.expr;

   // erase traces on the other side
   from.type = EXIT;
   from.lhs = 0;
   from.expr = 0;
}

} // namespace ir
