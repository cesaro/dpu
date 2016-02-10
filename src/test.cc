
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <array>
#include <algorithm>

#include "test.hh"
#include "ir.hh"
#include "pes.hh"

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
   t->addr = 3;
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
   ir::Trans * t;
   // load the program -> new machine
   ir::Machine m (3, 3, 3); // 3 vars, 3 procs, 3 trans

   ir::Process & p0 = m.add_process (2,0); // 2 locations
   t = & m.add_trans (p0, 0, 1); // p0 has one transition (WR), write on var 3
      t->type   = ir::Trans::WR;
      t->addr   = 3;
      t->offset = 0;

   ir::Process & p1 = m.add_process (2,1);
   t = & m.add_trans (p1, 0, 1); // p1 has one transition
      t->type   = ir::Trans::RD;
      t->addr   = 3;
      t->offset = 0;

   ir::Process & p2 = m.add_process (2,2);
   t = & m.add_trans (p2, 0, 1); // p2 has only one trans
      t->type   = ir::Trans::RD;
      t->addr   = 3;
      t->offset = 0;

   printf ("trans.size %zu\n", m.trans.size());
   printf ("procs.size %zu\n", m.procs.size());

   for (auto &t : m.trans)
      printf(" in test: t.src: %d and t.dest: %d, t.proc.id: %d \n", t.src, t.dst, t.proc.id);

   //const ir::State & s (m.init_state);

   pes::Unfolding u (m);
   /*
   u.evt=0;
   Config C(u); // C contains bottom event
   std::vector <Event *> D, A;
   u.explore(C, D, A);
   */

   u.explore_rnd_config ();

}
