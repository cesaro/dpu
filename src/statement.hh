
#ifndef __STATEMENT_HH_
#define __STATEMENT_HH_

#include <string>
#include <cstdint>

namespace ir
{

class Expr;

class Var // left-hand-side expression (variable or array)
{
public:
   unsigned var; // the variable, or the array base if idx != NULL
   Expr * idx; // the index of the array

   enum type_t {VAR, ARRAY};

   // factory methods
   static Var * make (unsigned tab = 0, Expr * = 0);

   Var * clone () const;
   Var & operator = (const Var & other);
   Var & operator = (Var && other);
   ~Var ();

   type_t type () const;
   std::string str () const;

private:

   Var (unsigned tab = 0, Expr * idx = 0);
   Var (const Var & other);
   //Var (Var && other);

   void steal (Var & from);
};

class Expr
{
public:
   enum type_t {VAR, IMM, OP1, OP2} type;
   enum op_t { ADD, SUB, DIV, EQ, LE, LT, AND, OR, NOT };
   union {
      struct { Var * v; };
      struct { int32_t imm; };
      struct {
         op_t op;
         Expr * expr1;
         Expr * expr2;
      };
   };

   // factory methods
   static Expr * make (int32_t imm = 0);
   static Expr * make (Var * v);
   static Expr * make (op_t o, Expr * e1);
   static Expr * make (op_t o, Expr * e1, Expr * e2);

   Expr * clone () const;
   Expr & operator = (const Expr & other);
   Expr & operator = (Expr && other);
   ~Expr ();

   std::string str () const;

private:

   // private constructors
   Expr ();
   Expr (const Expr & other);
   //Expr (Expr && other);

   int op_arity () const;
   const char * op_str () const;
   const char * type_str () const;
   void steal (Expr & from);
};

class Statement
{
public:
   enum {ASGN, ASSUME, LOCK, UNLOCK, EXIT} type;
   Var * lhs;
   Expr * expr;
};

} // namespace ir

#endif

