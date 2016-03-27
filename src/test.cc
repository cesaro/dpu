
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <array>
#include <memory>
#include <algorithm>
#include <sys/stat.h>

#include "test.hh"
#include "ir.hh"
#include "pes.hh"
#include "statement.hh"
#include "verbosity.h"

#include "fe/ir.hh"

using namespace dpu;

//#include "boost/filesystem.hpp"
//using namespace boost::filesystem;

std::unique_ptr<ir::Machine> build_concur15_example ();
std::unique_ptr<ir::Machine> build_mul_example ();
std::unique_ptr<ir::Machine> build_mul_example1 ();


class Stack
{
   int * tab;
   int len;
   int cap;

public:
   Stack ();
   Stack (const Stack &);
   ~Stack ();
   Stack & operator= (const Stack &);

   void * operator new (size_t);
   void * operator new[] (size_t);
   void operator delete (void *);

   void print ();
   int size ();
   int peek ();
};

Stack::Stack ()
{
   printf ("%p: Stack.ctor ()\n", this);
   cap = 10;
   len = 0;
   tab = new int [cap];
}

Stack::Stack (const Stack &s)
{
   printf ("%p: Stack.ctor: copy %p\n", this, &s);
   len = s.len;
   cap = s.cap;
   tab = new int[cap];
   for (int i = 0; i < len; i++) tab[i] = s.tab[i];
}

Stack & Stack::operator= (const Stack &s)
{
   printf ("%p: op=: copy %p\n", this, &s);
   delete [] tab;
   len = s.len;
   cap = s.cap;
   tab = new int[cap];
   for (int i = 0; i < len; i++) tab[i] = s.tab[i];
   return *this;
}

Stack::~Stack ()
{
   printf ("%p: Stack.dtor\n", this);
   delete [] tab;
}

int Stack::size ()
{
   printf ("%p: Stack.size: %d\n", this, len);
   return len;
}

#if 0
void * Stack::operator new (size_t size)
{
   printf ("Stack: operator new: size %zd\n", size);
   return (void *) new char [size];
}

void * Stack::operator new [] (size_t size)
{
   printf ("Stack: operator new[]: size %zd\n", size);
   return (void *) new char [size];
}

void Stack::operator delete (void * ptr)
{
   printf ("Stack: operator delete: ptr %p\n", ptr);
}
#endif

#if 0
void * operator new (std::size_t s)
{
   printf ("global op new: s %zu\n", s);
   return std::malloc (s);
}

void * operator new [] (std::size_t s)
{
   printf ("global op new[]: s %zu\n", s);
   return std::malloc (s);
}

void operator delete(void* ptr) noexcept
{
   std::printf("global op delete\n");
   std::free(ptr);
}
#endif
#if 0
void test1 ()
{
   Stack * s = new Stack [10];

   printf ("s %p\n", s);
   printf ("s.size %d\n", s->size ());

   int* p3 = new int[10];
   printf ("%d\n", *p3);
   delete[] p3;
}
#endif

void test2 ()
{
   std::vector<Stack> tab;

   printf ("size %zd\n", sizeof (Stack));

   std::printf ("emplace_back:\n");
   tab.emplace_back ();
   std::printf ("tab[0]:\n");
   tab[0].size ();

   std::printf ("emplace_back:\n");
   tab.emplace_back ();
   std::printf ("done\n");

#if 0
   tab.emplace_back ();
   tab.emplace_back ();
   tab.emplace_back ();
   tab.emplace_back ();
   std::printf ("tab[0] second time =======\n");
   tab[0].size ();
#endif
}

class MyTest
{
public:
   union
   {
      int a;
      float b;
   };
};

void test3 ()
{
   MyTest m;

   m.a = 123;
   printf ("m.a %d\n", m.a);

   m.b = 4.34;
   printf ("m.a %d\n", m.a);

   m.b = 4.35;
   printf ("m.a %d\n", m.a);

   std::vector<int> v;

   printf ("sizeof std::vector %zd\n", sizeof (v));
}

template<class T>
size_t get_size (T)
{
   return std::tuple_size<T>::value;
}

template< int N >
class MyTest2
{
public:
   std::array<int,N> a;
};

void test4 ()
{
   std::array<int,3> v;
   v[0] = 1;
   v[1] = 1;
   v[2] = 1;

   printf ("array size %zd\n", get_size (v));
   printf ("MyTest2<0> size %zd\n", sizeof (MyTest2<0>));
   printf ("MyTest2<1> size %zd\n", sizeof (MyTest2<1>));
   printf ("MyTest2<10> size %zd\n", sizeof (MyTest2<10>));
}

void test5 ()
{
   ir::Machine m (3, 4, 3);
#if 0
   ir::Process & p0 = m.add_process (2);
   ir::Process & p1 = m.add_process (2);
   ir::Process & p2 = m.add_process (2);

   ir::Trans * t;
   
   t = & m.add_trans (p0, 0, 1);
   t->type = ir::Trans::WR;
   t->var = 3;
   t->offset = 0;

   t = & m.add_trans (p1, 0, 1);
   t = & m.add_trans (p2, 0, 1);

   const ir::State & s (m.init_state);

   for (unsigned var = 0; var < m.memsize; var++)
   {
     printf ("variable %d stores '%u'\n", var, s[var]);
   }

#endif
   // x state

   // pensar en el formato objetivo que necesitas, formato en memoria; en
   // particular, como haces los branches; despues comprender el LLVMIR en
   // detalle, ver como se las apaña para los branches y asegurarte de que la
   // conversion es posible (y sencilla); permitir threads no deterministas en
   // el formato pero rechazarlos en la version actual del codigo

   // diseñar la clase Codeblock
   // enabled
   // fire

   //printf ("s.tab %p\n", s.tab);
   //printf ("s.m %p\n", s.m);
}

void test6 ()
{

   std::vector<int> v {1, 2, 3, 4, 5};

   auto it = std::find (v.begin(), v.end(), 3);
   if (it != v.end ())
      printf ("find %d\n", *it);
   else
      printf ("find didn't find\n");
}

void test7 ()
{
   auto m = build_concur15_example ();

   DEBUG ("\n%s", m->str().c_str());

   pes::Unfolding u (*m.get ());
   /* Explore a random configuration */
   u.explore_rnd_config ();
}

void test8 ()
{
   // testing ctors and assignemnt ops for the Var class

   {
      printf ("======= v0\n");
      ir::Var * v = ir::Var::make ();
      printf ("v.var %u\n", v->var);
      printf ("v.idx %p\n", v->idx);
      printf ("v.type %u\n", v->type ());
      printf ("v.str '%s'\n", v->str().c_str());
      delete v;
   }

   {
      printf ("======= v12\n");
      std::unique_ptr<ir::Var> v (ir::Var::make (12));
      printf ("v.var %u\n", v->var);
      printf ("v.idx %p\n", v->idx);
      printf ("v.type %u\n", v->type ());
      printf ("v.str '%s'\n", v->str().c_str());
   }

   {
      printf ("======= v12[0]\n");
      ir::Expr * e = ir::Expr::make (0);
      std::unique_ptr<ir::Var> v (ir::Var::make (12, e));
      printf ("v.var %u\n", v->var);
      printf ("v.idx %p\n", v->idx);
      printf ("v.type %u\n", v->type ());
      printf ("v.str '%s'\n", v->str().c_str());

      printf ("======= copy ctor of Var\n");
      std::unique_ptr<ir::Var> v1 (v->clone ());
      printf ("v1.var  %u\n",   v1->var);
      printf ("v1.idx  %p\n",   v1->idx);
      printf ("v1.type %u\n",   v1->type ());
      printf ("v1.str  '%s'\n", v1->str().c_str());
   }

#if 0
   {
      printf ("======= v1 with move ctor\n");
      ir::Expr imm33 (33);
      ir::Var v (1, imm33);
      printf ("v.var %u\n",  v->var);
      printf ("v.idx %p\n",  v->idx);
      printf ("v.type %u\n", v->type ());
      printf ("v.str '%s'\n",v->str().c_str());
      ir::Var v1 (std::move (v)); // declares v as r-value
      printf ("v.var %u\n",  v->var);
      printf ("v.idx %p\n",  v->idx);
      printf ("v.type %u\n", v->type ());
      printf ("v.str '%s'\n",v->str().c_str());
      printf ("v1.var %u\n",v1->var);
      printf ("v1.idx %p\n",v1->idx);
      printf ("v1.type %u\n", v1->type ());
      printf ("v1.str '%s'\n",v1->str().c_str());
   }
#endif

   {
      printf ("======= v1 with move assignment\n");
      ir::Expr * imm33 = ir::Expr::make (33);
      ir::Var * v = ir::Var::make (1, imm33);
      printf ("v.var %u\n",  v->var);
      printf ("v.idx %p\n",  v->idx);
      printf ("v.type %u\n", v->type ());
      printf ("v.str '%s'\n",v->str().c_str());

      // move assignment
      ir::Var * v123 = ir::Var::make (123);
      *v = std::move (* v123);
      printf ("v.var %u\n",  v->var);
      printf ("v.idx %p\n",  v->idx);
      printf ("v.type %u\n", v->type ());
      printf ("v.str '%s'\n",v->str().c_str());
      delete v;
      printf ("deleting v123:\n");
      delete v123;
   }
}

void test9 ()
{
   // testing ctors and assignemnt ops for the Expr class

   printf ("======= '-13'\n");
   ir::Expr * e13 = ir::Expr::make (-13);
   //printf ("e->v    '%s'\n", e->str().c_str());
   printf ("e->imm  %d\n",   e13->imm);
   printf ("e->type '%s'\n", e13->type_str ());
   printf ("e->str  '%s'\n", e13->str().c_str());
   //delete e13;

   printf ("======= 'v10'\n");
   ir::Expr * v10 = ir::Expr::make (ir::Var::make (10));
   printf ("e->v    '%s'\n", v10->str().c_str());
   printf ("e->type '%s'\n", v10->type_str ());
   printf ("e->str  '%s'\n", v10->str().c_str());
   // delete v10; // should delete the expression and the variable (OK)

   printf ("======= 'not v10'\n");
   ir::Expr * not_v10 = ir::Expr::make (ir::Expr::NOT, v10);
   printf ("e->type '%s'\n", not_v10->type_str ());
   printf ("e->str  '%s'\n", not_v10->str().c_str());
   //delete not_v10;

   printf ("======= '(not v10) + (-13)'\n");
   ir::Expr * plus = ir::Expr::make (ir::Expr::ADD, not_v10, e13);
   printf ("e->type '%s'\n", plus->type_str ());
   printf ("e->str  '%s'\n", plus->str().c_str());
   //delete plus;

   printf ("======= '((not v10) + (-13)) == v10'\n");
   ir::Expr * v10dup = v10->clone ();
   ir::Expr * eq = ir::Expr::make (ir::Expr::EQ, plus, v10dup);
   printf ("e->type '%s'\n", eq->type_str ());
   printf ("e->str  '%s'\n", eq->str().c_str());
   delete eq;
}

void test10 ()
{
   printf ("======= v0 = 1\n");
   ir::Var  * v0   = ir::Var::make (0);
   ir::Expr * int1 = ir::Expr::make (1);
   ir::Stm stm_v0 (ir::Stm::ASGN, v0, int1);
   printf ("s->str  '%s'\n", stm_v0.str().c_str());

   printf ("======= assume (v0)\n");
   ir::Expr * v0_1 = ir::Expr::make (v0->clone ());
   ir::Stm assume (ir::Stm::ASSUME, v0_1);
   printf ("s->str  '%s'\n", assume.str().c_str());

   printf ("======= lock (v1)\n");
   ir::Stm lck (ir::Stm::LOCK, ir::Var::make (1));
   printf ("s->str  '%s'\n", lck.str().c_str());

   printf ("======= exit\n");
   ir::Stm ex (ir::Stm::EXIT);
   printf ("s->str  '%s'\n", ex.str().c_str());
}

void test11 ()
{
   std::vector<int> v { 1, 2, 3, 4 };
   printf ("%d %d\n", v[0], v[1]);

   for (auto & i : v) i = 123;
   printf ("%d %d\n", v[0], v[1]);
}

std::unique_ptr<ir::Machine> build_mul_example ()
{
   /*
    * One thread, multiplies v2 times v1 (the inputs) using a loop and checks
    * that the result is the same as if using the instruction MUL.
    *
    * memsize  = 5 = 4 vars + 1 pc (v0)
    * numprocs = 1
    * numtrans = 12
    *
    * src dst  what
    *   0   1  v1 = 2
    *   1   2  v2 = 5
    *   2   3  v3 = 0
    *   3   4  v4 = 0
    *   4   5  assume (v3 < v2)
    *   5   6  v4 = v4 + v1
    *   6   4  v3 = v3 + 1
    *   4   7  assume (v3 >= v2)
    *   7   8  assume (v4 != v1 * v2)
    *   8   9  error
    *   7  10  assume (v4 == v1 * v2)
    *  10  11  exit
    *
    */

   ir::Trans * t;
   std::unique_ptr<ir::Machine> m (new ir::Machine (5, 1, 12)); // 5 vars, 1 thread, 12 transitions
   ir::Process & p = m->add_process (12); // 12 locations in this thread

   // variables v1 to v4
   std::unique_ptr<ir::Var> v1 (ir::Var::make (1));
   std::unique_ptr<ir::Var> v2 (ir::Var::make (2));
   std::unique_ptr<ir::Var> v3 (ir::Var::make (3));
   std::unique_ptr<ir::Var> v4 (ir::Var::make (4));

   //  0 >  1 : v1 = 2
   t = & p.add_trans (0, 1);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v1->clone (), ir::Expr::make (2));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(1);

   //  1 >  2 : v2 = 5
   t = & p.add_trans (1, 2);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v2->clone (), ir::Expr::make (5));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(2);

   //  2 >  3 : v3 = 0
   t = & p.add_trans (2, 3);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v3->clone (), ir::Expr::make (0));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(3);

   //  3 >  4 : v4 = 0
   t = & p.add_trans (3, 4);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v4->clone (), ir::Expr::make (0));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(4);

   //  4 >  5 : assume (v3 < v2)
   t = & p.add_trans (4, 5);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::LT, ir::Expr::make (v3->clone()), ir::Expr::make (v2->clone ())));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(2);
   t->localvars.push_back(3);

   //  5 >  6 : v4 = v4 + v1
   t = & p.add_trans (5, 6);
   t->code.stm = ir::Stm (ir::Stm::ASGN,
         v4->clone (),
         ir::Expr::make (ir::Expr::ADD, ir::Expr::make (v4->clone()), ir::Expr::make (v1->clone ())));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(1);
   t->localvars.push_back(4);

   //  6 >  4 : v3 = v3 + 1
   t = & p.add_trans (6, 4);
   t->code.stm = ir::Stm (ir::Stm::ASGN,
         v3->clone (),
         ir::Expr::make (ir::Expr::ADD, ir::Expr::make (v3->clone()), ir::Expr::make (1)));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(3);

   //  4 >  7 : assume (v3 >= v2)
   t = & p.add_trans (4, 7);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::LE, ir::Expr::make (v2->clone()), ir::Expr::make (v3->clone ())));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(2);
   t->localvars.push_back(2);

   //  7 >  8 : assume (v4 != v1 * v2)
   t = & p.add_trans (7, 8);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::NE,
            ir::Expr::make (v4->clone ()),
            ir::Expr::make (ir::Expr::MUL, ir::Expr::make (v1->clone()), ir::Expr::make (v2->clone ()))));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars={4,1,2};

   //  8 >  9 : error
   t = & p.add_trans (8, 9);
   t->code.stm = ir::Stm (ir::Stm::ERROR);
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.clear();

   //  7 > 10 : assume (v4 == v1 * v2)
   t = & p.add_trans (7, 10);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::EQ,
            ir::Expr::make (v4->clone ()),
            ir::Expr::make (ir::Expr::MUL, ir::Expr::make (v1->clone()), ir::Expr::make (v2->clone ()))));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars = {4,1,2};


   // 10 > 11 : exit
   t = & p.add_trans (10, 11);
   t->code.stm = ir::Stm (ir::Stm::EXIT);
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.clear();

   return m;
}

std::unique_ptr<ir::Machine> build_mul_example2 ()
{
   /*
    * One thread, multiplies v2 times v1 (the inputs) using a loop and checks
    * that the result is the same as if using the instruction MUL.
    * 
    * memsize  = 6 = 4 vars + 2 pc
    * numprocs = 2
    * numtrans = 14
    *
    * v1 is a global variable, others are local.
    *
    * Process 0:
    * src dst  what
    *   0   1  v1 = 2
    *   1   2  v2 = 5
    *   2   3  v3 = 0
    *   3   4  v4 = 0
    *   4   5  assume (v3 < v2)
    *   5   6  v4 = v4 + v1
    *   6   4  v3 = v3 + 1
    *   4   7  assume (v3 >= v2)
    *   7   8  assume (v4 != v1 * v2)
    *   8   9  error
    *   7  10  assume (v4 == v1 * v2)
    *  10  11  exit
    *
    *  Process 1:
    *  src dst  what
    *   0   1  v1 = 123
    *   1   2  exit
    *
    */

   ir::Trans * t;
   std::unique_ptr<ir::Machine> m (new ir::Machine (6, 2, 14)); // 6 mems (4 var + 2 pc), 2 thread, 14 transitions
   ir::Process & p  = m->add_process (12); // 12 locations in this thread
   ir::Process & p1 = m->add_process (3); // 3 locations in this thread

   // variables v2 to v5
   std::unique_ptr<ir::Var> v2 (ir::Var::make (2));
   std::unique_ptr<ir::Var> v3 (ir::Var::make (3));
   std::unique_ptr<ir::Var> v4 (ir::Var::make (4));
   std::unique_ptr<ir::Var> v5 (ir::Var::make (5));

   /* Process 0: */
   //  0 >  1 : v2 = 2
   t = & p.add_trans (0, 1);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v2->clone (), ir::Expr::make (2));
   t->type = ir::Trans::WR;
   t->var = 2;
   t->offset = 0;
   t->localvars.clear ();

   //  1 >  2 : v3 = 5
   t = & p.add_trans (1, 2);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v3->clone (), ir::Expr::make (5));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(3);

   //  2 >  3 : v4 = 0
   t = & p.add_trans (2, 3);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v4->clone (), ir::Expr::make (0));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(4);

   //  3 >  4 : v5 = 0
   t = & p.add_trans (3, 4);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v5->clone (), ir::Expr::make (0));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(5);

   //  4 >  5 : assume (v4 < v3)
   t = & p.add_trans (4, 5);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::LT, ir::Expr::make (v4->clone()), ir::Expr::make (v3->clone ())));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(3);
   t->localvars.push_back(4);


   //  5 >  6 : v5 = v5 + v2
   t = & p.add_trans (5, 6);
   t->code.stm = ir::Stm (ir::Stm::ASGN,
         v5->clone (),
         ir::Expr::make (ir::Expr::ADD, ir::Expr::make (v5->clone()), ir::Expr::make (v2->clone ())));
   t->type = ir::Trans::RD;
   t->var = 2;
   t->offset = 0;
   t->localvars.push_back(5);

   //  6 >  4 : v4 = v4 + 1
   t = & p.add_trans (6, 4);
   t->code.stm = ir::Stm (ir::Stm::ASGN,
         v4->clone (),
         ir::Expr::make (ir::Expr::ADD, ir::Expr::make (v4->clone()), ir::Expr::make (1)));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(4);

   //  4 >  7 : assume (v4 >= v3)
   t = & p.add_trans (4, 7);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::LE, ir::Expr::make (v3->clone()), ir::Expr::make (v4->clone ())));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(3);
   t->localvars.push_back(4);



   //  7 >  8 : assume (v5 != v2 * v3)
   t = & p.add_trans (7, 8);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::NE,
            ir::Expr::make (v5->clone ()),
            ir::Expr::make (ir::Expr::MUL, ir::Expr::make (v2->clone()), ir::Expr::make (v3->clone ()))));
   t->type = ir::Trans::RD;
   t->var = 2;
   t->offset = 0;
   t->localvars={3};
   t->localvars={5};

   //  8 >  9 : error
   t = & p.add_trans (8, 9);
   t->code.stm = ir::Stm (ir::Stm::ERROR);
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.clear();

   //  7 > 10 : assume (v5 == v2 * v3)
   t = & p.add_trans (7, 10);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::EQ,
            ir::Expr::make (v5->clone ()),
            ir::Expr::make (ir::Expr::MUL, ir::Expr::make (v2->clone()), ir::Expr::make (v3->clone ()))));
   t->type = ir::Trans::RD;
   t->var = 2;
   t->offset = 0;
   t->localvars={3};
   t->localvars={5};

   // 10 > 11 : exit
   t = & p.add_trans (10, 11);
   t->code.stm = ir::Stm (ir::Stm::EXIT);
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.clear();

   /* Process 1 */
   //  0 >  1 : v2 = 123
   t = & p1.add_trans (0, 1);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v2->clone (), ir::Expr::make (123));
   t->type = ir::Trans::WR;
   t->var = 2;
   t->offset = 0;
   t->localvars.clear ();

   //  1 >  2 : exit
   t = & p1.add_trans (1, 2);
   t->code.stm = ir::Stm (ir::Stm::EXIT);
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.clear();

   return m;
}

std::unique_ptr<ir::Machine> build_mul_example1 ()
{
   /*
    * Two threads:
    * One thread: multiplies v2 times v1 (the inputs) using a loop and checks
    * that the result is the same as if using the instruction MUL.
    * Another thread: assign a value to v1
    * v1 is a global variable.
    *
    * memsize  = 6 = 4 vars + 2 pc (v0, v1)
    * numprocs = 2
    * numtrans = 14
    *
    * src   dst   what
    *   0    1    v1 = 2
    *   1    2    v2 = 5
    *   2    3    v3 = 0
    *   3    4    v4 = 0
    *   4    5    assume (v3 < v2)
    *   5    6    v4 = v4 + v1
    *   6    4    v3 = v3 + 1
    *   4    7    assume (v3 >= v2)
    *   7    8    assume (v4 != v1 * v2)
    *   8    9    error
    *   7   10    assume (v4 == v1 * v2)
    *  10   11    exit
    *
    * second thread:
    * src   dst   what
    *   0   1     v1 = 123
    *   1   2     EXIT
    */

   ir::Trans * t;
   std::unique_ptr<ir::Machine> m (new ir::Machine (6, 2, 14)); // 6 mem (4 vars + 2 thread), 2 threads, 14 transitions
   ir::Process & p   = m->add_process (12); // 12 locations in this thread
   ir::Process & p1  = m->add_process(3); // 3 locations in this thread

   // variables v1 to v4
   std::unique_ptr<ir::Var> v1 (ir::Var::make (1));
   std::unique_ptr<ir::Var> v2 (ir::Var::make (2));
   std::unique_ptr<ir::Var> v3 (ir::Var::make (3));
   std::unique_ptr<ir::Var> v4 (ir::Var::make (4));

   //  0 >  1 : v1 = 2
   t = & p.add_trans (0, 1);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v1->clone (), ir::Expr::make (2));
   t->type = ir::Trans::WR;
   t->var = 2;
   t->offset = 0;
   t->localvars.clear();

   //  1 >  2 : v2 = 5
   t = & p.add_trans (1, 2);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v2->clone (), ir::Expr::make (5));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(3);

   //  2 >  3 : v3 = 0
   t = & p.add_trans (2, 3);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v3->clone (), ir::Expr::make (0));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(4);

   //  3 >  4 : v4 = 0
   t = & p.add_trans (3, 4);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v4->clone (), ir::Expr::make (0));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(5);

   //  4 >  5 : assume (v3 < v2)
   t = & p.add_trans (4, 5);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::LT, ir::Expr::make (v3->clone()), ir::Expr::make (v2->clone ())));

   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars = {3,4};

   //  5 >  6 : v4 = v4 + v1
   t = & p.add_trans (5, 6);
   t->code.stm = ir::Stm (ir::Stm::ASGN,
         v4->clone (),
         ir::Expr::make (ir::Expr::ADD, ir::Expr::make (v4->clone()), ir::Expr::make (v1->clone ())));
   t->type = ir::Trans::RD;
   t->var = 2;
   t->offset = 0;
   t->localvars.push_back(5);

   //  6 >  4 : v3 = v3 + 1
   t = & p.add_trans (6, 4);
   t->code.stm = ir::Stm (ir::Stm::ASGN,
         v3->clone (),
         ir::Expr::make (ir::Expr::ADD, ir::Expr::make (v3->clone()), ir::Expr::make (1)));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.push_back(4);

   //  4 >  7 : assume (v3 >= v2)
   t = & p.add_trans (4, 7);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::LE, ir::Expr::make (v2->clone()), ir::Expr::make (v3->clone ())));
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars={3,4};


   //  7 >  8 : assume (v4 != v1 * v2)
   t = & p.add_trans (7, 8);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
       ir::Expr::make (ir::Expr::NE,
          ir::Expr::make (v4->clone ()),
             ir::Expr::make (ir::Expr::MUL, ir::Expr::make (v1->clone()), ir::Expr::make (v2->clone ()))));
   t->type = ir::Trans::RD;
   t->var = 2;
   t->offset = 0;
   t->localvars={5,3};

   //  8 >  9 : error
   t = & p.add_trans (8, 9);
   t->code.stm = ir::Stm (ir::Stm::ERROR);
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.clear();

   //  7 > 10 : assume (v4 == v1 * v2)
   t = & p.add_trans (7, 10);
   t->code.stm = ir::Stm (ir::Stm::ASSUME,
         ir::Expr::make (ir::Expr::EQ,
            ir::Expr::make (v4->clone ()),
            ir::Expr::make (ir::Expr::MUL, ir::Expr::make (v1->clone()), ir::Expr::make (v2->clone ()))));
   t->type = ir::Trans::RD;
   t->var = 2;
   t->offset = 0;
   t->localvars={5,3};

   // 10 > 11 : exit
   t = & p.add_trans (10, 11);
   t->code.stm = ir::Stm (ir::Stm::EXIT);
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.clear();

/* Thread 2 */
   //  0 >  1 : v1 = 123
   t = & p1.add_trans (0, 1);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v1->clone (), ir::Expr::make (123));
   t->type = ir::Trans::WR;
   t->var = 2;
   t->offset = 0;
   t->localvars.clear();

   //  1 >  2 : EXIT
   t = & p1.add_trans (1, 2);
   t->code.stm = ir::Stm (ir::Stm::EXIT);
   t->type = ir::Trans::LOC;
   t->offset = 0;
   t->localvars.clear();

   return m;
}


std::unique_ptr<ir::Machine> build_concur15_example ()
{
   /*
    * Three threads, as in the CONCUR'15 paper with x=v3, y=v4, z=v5
    * 
    * memsize  = 6 = 3 vars + 3 pcs
    * numprocs = 3
    * numtrans = 6
    *
    * Thread 0
    * ========
    * src dst  what           type  var  localvars
    *   0   1  v3 = 123       WR    3     {}
    *   1   2  exit           LOC   na    {}
    *
    * Thread 1
    * ========
    * src dst  what           type  var  localvars
    *   0   1  v4 = v3 + 1    RD    3     {4}
    *   1   2  exit           LOC   na    {}
    *
    * Thread 2
    * ========
    * src dst  what           type  var  localvars
    *   0   1  v5 = v3 + 2;   RD    3     {5}
    *   1   2  exit           LOC   na    {}
    *
    */

   ir::Trans * t;
   std::unique_ptr<ir::Machine> m (new ir::Machine (6, 3, 6)); // 6 vars, 3 thread, 6 transitions
   ir::Process & p0 = m->add_process (3); // 3 locations per thread
   ir::Process & p1 = m->add_process (3);
   ir::Process & p2 = m->add_process (3);

   // variables v1 to v4
   std::unique_ptr<ir::Var> v3 (ir::Var::make (3));
   std::unique_ptr<ir::Var> v4 (ir::Var::make (4));
   std::unique_ptr<ir::Var> v5 (ir::Var::make (5));

   //   0   1  v3 = 123
   t = & p0.add_trans (0, 1);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v3->clone (), ir::Expr::make (123));
   t->type = ir::Trans::WR;
   t->var = 3;
   t->offset = 0;
   t->localvars.clear ();

   //   1   2  exit
   t = & p0.add_trans (1, 2);
   t->code.stm = ir::Stm (ir::Stm::EXIT);
   t->type = ir::Trans::LOC;
   t->localvars.clear ();

   //   0   1  v4 = v3 + 1
   t = & p1.add_trans (0, 1);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v4->clone (),
         ir::Expr::make (ir::Expr::ADD, ir::Expr::make (v3->clone()), ir::Expr::make (1)));
   t->type = ir::Trans::RD;
   t->var = 3;
   t->offset = 0;
   t->localvars.push_back (4);

   //   1   2  exit
   t = & p1.add_trans (1, 2);
   t->code.stm = ir::Stm (ir::Stm::EXIT);
   t->type = ir::Trans::LOC;
   t->localvars.clear ();

   //   0   1  v5 = v3 + 2;
   t = & p2.add_trans (0, 1);
   t->code.stm = ir::Stm (ir::Stm::ASGN, v5->clone (),
         ir::Expr::make (ir::Expr::ADD, ir::Expr::make (v3->clone()), ir::Expr::make (2)));
   t->type = ir::Trans::RD;
   t->var = 3;
   t->offset = 0;
   t->localvars.push_back (5);

   //   1   2  exit
   t = & p2.add_trans (1, 2);
   t->code.stm = ir::Stm (ir::Stm::EXIT);
   t->type = ir::Trans::LOC;
   t->localvars.clear ();

   return m;
}

void test12 ()
{
   //auto m = build_mul_example ();
   auto m = build_concur15_example ();

   DEBUG ("%s", m->str().c_str());

   ir::State s (m->init_state);
   std::vector<ir::Trans*> ena;
   unsigned seed, i;

   seed = std::time (0); // use current time as seed for random
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
      DEBUG ("firing nr %u", i);
      ena[i]->fire (s);
   }
}

void test13 ()
{
#if 0
   const char * st = "foo";
   const int dir_err = mkdir(st, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
   if (-1 == dir_err)
   {
       printf("Error creating directory!n");
       exit(1);
   }
#endif

}

void test14()
{
   auto m = build_mul_example2 ();
   DEBUG ("\n%s", m->str().c_str());

   pes::Unfolding u (*m.get ());

   /* Explore a driven configuration */
   u.explore_driven_config ();
}

void test15()
{
   auto m = build_concur15_example ();

   DEBUG ("\n%s", m->str().c_str());

   pes::Unfolding u (*m.get ());
   /* Explore a random configuration */
   u.explore_rnd_config ();
}

void test16()
{
	using namespace fe::ir;

	Function * f;

	Program p (1);

	// add 1 thread
	f = p.add_thread ("main");
	p.main = f;

	// allocate 5 symbols of 32 bits
	Symbol * x = p.module.allocate ("x", 4, 4, 4);
	Symbol * y = p.module.allocate ("y", 4, 4, 20);
	//Symbol * i = p.module.allocate ("i", 4, 4, 128);
	//Symbol * acc = p.module.allocate ("acc", 4, 4);
	//Symbol * cnd = p.module.allocate ("cnd", 4, 4);
	p.module.allocate ("xx", 5, 8, 0x1234);
	p.module.allocate ("yy", 10, 16, 0x12345678);
	Symbol * fmt = p.module.allocate ("fmt", 16, 1, "hello world %d");

	p.dump ();

	Instr ins;
	ins.op = Opcode::RET;
	ins.size = 1;
	ins.src1 = Addr (*x) + 0x1;
	//ins.src2 = 0x0123456789abcef;
	ins.src2 = 0x0123;
	std::cout << p.module.print_instr (&ins) << "\n";

	ins.op = Opcode::MOVE;
	ins.size = 1;
	ins.dst = Addr (*y) + 0x5;
	ins.src1 = 0x1;
	ins.src2 = 0x7; // ignored, illegal
	std::cout << p.module.print_instr (&ins) << "\n";

	ins.op = Opcode::MOVEI;
	ins.size = 8;
	ins.dst = 0x5;
	ins.src2 = 0x5566;
	std::cout << p.module.print_instr (&ins) << "\n";

	ins.op = Opcode::PRINTF;
	ins.size = 4;
	ins.dst = Addr (*fmt);
	ins.src1 = Addr (*y);
	ins.src2 = 0;
	std::cout << p.module.print_instr (&ins) << "\n";

	return;
}

void test17 ()
{
	using namespace fe::ir;

	Function * f;

	Program p (1);

	// add 1 thread
	f = p.add_thread ("main");
	p.main = f;

	// create an instruction builder attached to the main function
	Builder b (f);

	// allocate 5 symbols, 32 bits each one
	Symbol * x = p.module.allocate ("x", 4, 4, 4);
	Symbol * y = p.module.allocate ("y", 4, 4, 20);
	Symbol * i = p.module.allocate ("i", 4, 4);
	Symbol * acc = p.module.allocate ("acc", 4, 4);

	// generate some instructions
	b.mk_error ();
	b.set_comment ("error instructions");

	b.mk_ret (I16, Addr (0x2));
	b.set_comment ("return instructions");
	b.mk_ret (I32, Imm (0x1234));

	b.mk_move (I32, *x, Imm (2));
	b.set_comment ("instructions for moving data");
	b.mk_move (I32, *y, *i);
	b.mk_movis (I64, *acc, *x);
	b.mk_movid (I64, *acc, *x);

	b.mk_cmp_eq (I16, *acc, *x, *y);
	b.set_comment ("comparison instructions");
	b.mk_cmp_eq (I16, *acc, *x, Imm (129));

	b.mk_cmp_ne (I16, *acc, *x, *y);
	b.mk_cmp_ne (I16, *acc, *x, Imm (129));
	b.mk_cmp_ugt (I16, *acc, *x, *y);
	b.mk_cmp_ugt (I16, *acc, *x, Imm (129));
	b.mk_cmp_uge (I16, *acc, *x, *y);
	b.mk_cmp_uge (I16, *acc, *x, Imm (129));
	b.mk_cmp_ult (I16, *acc, *x, *y);
	b.mk_cmp_ult (I16, *acc, *x, Imm (129));
	b.mk_cmp_ule (I16, *acc, *x, *y);
	b.mk_cmp_ule (I16, *acc, *x, Imm (129));
	b.mk_cmp_sgt (I16, *acc, *x, *y);
	b.mk_cmp_sgt (I16, *acc, *x, Imm (129));
	b.mk_cmp_sge (I16, *acc, *x, *y);
	b.mk_cmp_sge (I16, *acc, *x, Imm (129));
	b.mk_cmp_slt (I16, *acc, *x, *y);
	b.mk_cmp_slt (I16, *acc, *x, Imm (129));
	b.mk_cmp_sle (I16, *acc, *x, *y);
	b.mk_cmp_sle (I16, *acc, *x, Imm (129));

	b.mk_br (*acc);
	b.set_comment ("branch instruction");
	b.push ();

	// we continue generating instruction on the non-zero branch
	b.set_branch (1);

	b.mk_add (I8, *i, *x, *y);
	b.set_label (); // target of a branch needs a label
	b.set_comment ("arithmetic instructions with 2 addresses");
	b.mk_sub (I8, *i, *x, *y);
	b.mk_mul (I8, *i, *x, *y);
	b.mk_sdiv (I8, *i, *x, *y);
	b.mk_udiv (I8, *i, *x, *y);
	b.mk_srem (I8, *i, *x, *y);
	b.mk_urem (I8, *i, *x, *y);

	b.mk_add (I8, *i, *x, Imm (26));
	b.set_comment ("arithmetic instructions with one immediate value");
	b.set_comment ("i = x + 26");
	b.mk_sub (I8, *i, Imm (26), *x);
	b.set_comment ("i = 2 - x");
	b.mk_add (I8, *i, *x, Imm ((uint8_t) -2));
	b.set_comment ("i = x - 2");
	b.mk_mul (I16, *i, *x, Imm ((uint16_t) -2));
	b.set_comment ("i = x * -2");

	b.mk_sdiv (I8, *i, *x, Imm ((uint8_t) -1));
	b.set_comment ("i = x / -1");
	b.mk_sdiv (I64, *i, Imm ((uint8_t) -1), *x);
	b.set_comment ("i = -1 / x");
	b.mk_udiv (I8, *i, *x, Imm ((uint8_t) 32));
	b.set_comment ("i = x / 32");
	b.mk_udiv (I8, *i, Imm ((uint8_t) 32), *x);
	b.set_comment ("i = 32 / x");

	b.mk_srem (I8, *i, *x, Imm ((uint8_t) -2));
	b.set_comment ("i = x % -2");
	b.mk_srem (I8, *i, Imm ((uint8_t) -2), *x);
	b.set_comment ("i = -2 % x");
	b.mk_urem (I8, *i, *x, Imm ((uint8_t) 32));
	b.set_comment ("i = x % 32");
	b.mk_urem (I8, *i, Imm ((uint8_t) 32), *x);
	b.set_comment ("i = 32 % x");

	b.mk_ret (I64, Imm (0));

	b.pop (); // continue at the last branch instruction pushed
	b.set_branch (0); // continue adding instructions in zero branch

	b.mk_and (I64, *i, *x, *y);
	b.set_label (); // target of a BR instruction always needs a label
	b.set_comment ("bitwise logic instructions");
	b.mk_and (I64, *i, *x, Imm (0xff0800));
	b.mk_or (I64, *i, *x, *y);
	b.mk_or (I64, *i, *x, Imm (0xff0800));
	b.mk_xor (I64, *i, *x, *y);
	b.mk_xor (I64, *i, *x, Imm (0xff0800));

	b.mk_sext (I8, I16, *i, *x);
	b.set_comment ("sign extension");
	b.mk_zext (I16, I32, *i, *x);

	b.mk_lock (*acc);
	b.set_comment ("miscellaneous instructions");
	b.mk_unlock (*acc);
	b.mk_printf ("hello world without arguments");
	b.mk_printf ("format with one 32bit arg %d", I32, *x);
	b.mk_printf ("format with two 64bit args %ld and %ld", I64, *x, *y);

	p.dump ();
}

void test18 ()
{
	// build the following program:
	//
   //  int y = 5;
	//
   //  int main ()
   //  {
   //    int x = 2;
   //    int i;
   //    int acc = 0;
   // 
   //    for (i = 0; i < y; i++) acc += y
   // 
   //    assert (acc == x * y);
	//    return 0;
   //  }
   //
   //  int thr1 ()
   //  {
   //    y = 6;
	//    return 0;
   //  }

	using namespace fe::ir;

	Function * f;
	Instruction * ins1;
	Instruction * ins2;

	Program p (2);

	// add 1 thread
	f = p.add_thread ("main");
	p.main = f;

	// create an instruction builder attached to the main function
	Builder b (f);

	// allocate 5 symbols, 32 bits each one
	Symbol * x = p.module.allocate ("x", 4, 4, 4);
	Symbol * y = p.module.allocate ("y", 4, 4, 20);
	Symbol * i = p.module.allocate ("i", 4, 4);
	Symbol * acc = p.module.allocate ("acc", 4, 4);
	Symbol * cnd = p.module.allocate ("cnd", 4, 4);

	b.mk_move (I32, *x, Imm (2));
	b.set_label ("entry");
	b.mk_move (I32, *y, Imm (5));
	b.mk_move (I32, *i, Imm (0));

	ins1 = b.mk_cmp_slt (I32, *cnd, *i, *y);
	b.set_label ("loopend");

	// branch instruction, continue with the NZ branch
	b.mk_br (*cnd);
	b.push ();
	b.set_branch (1);

	b.mk_add (I32, *acc, *acc, *x);
	b.set_label ("loophead");
	ins2 = b.mk_add (I32, *i, *i, Imm (1));
	ins2->set_next (ins1);

	// at the branch instruction again, continue with the Z branch
	b.pop ();
	b.set_branch (0);

	b.mk_mul (I32, *i, *x, *y);
	b.set_label ("loopexit");
	b.mk_cmp_ne (I32, *cnd, *acc, *i);
	b.mk_br (*cnd);

	// push BR instruction, continue with NZ branch
	b.push ();
	b.set_branch (1);

	b.mk_error ();
	b.set_label ();

	// continue with Z branch
	b.pop ();
	b.set_branch (0);
	b.mk_ret (I32, Imm (0));
	b.set_label ();



	// add a second thread
	f = p.add_thread ("thr1");
	b.attach (f);

	b.mk_move (I32, *y, Imm (6));
	b.mk_ret (I32, Imm (0));

	p.dump ();
	//for (auto f : p.module) f->dump2 ();
	p.validate ();

	for (auto f : p.module) SHOW (f, "p");
	for (auto s = p.module.sym_begin(); s != p.module.sym_end(); ++s)
	{
		DEBUG ("Symbol '%s' addr %lu size %lu", (*s)->name.c_str(), (*s)->addr, (*s)->size);
	}
}

