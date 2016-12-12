
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <array>
#include <memory>
#include <algorithm>
#include <sys/stat.h>

#include "c15unfold.hh"
#include "verbosity.h"
#include "misc.hh"
#include "test.hh"
#include "pes.hh"
#include "por.hh"


using namespace dpu;

void test27 ()
{
   Unfolding u;

   SHOW (alignp2 (0), "zu");
   SHOW (alignp2 (1), "zu");
   SHOW (alignp2 (2), "zu");
   SHOW (alignp2 (4), "zu");
   SHOW (alignp2 (6), "zu");
   SHOW (alignp2 (8), "zu");
   SHOW (alignp2 (257), "zu");

   SHOW (int2mask (0), "zu");
   SHOW (int2mask (1), "zu");
   SHOW (int2mask (2), "zu");
   SHOW (int2mask (3), "zu");
   SHOW (int2mask (4), "zu");
   SHOW (int2mask (5), "zu");
   SHOW (int2mask (6), "zu");
   SHOW (int2mask (9), "zu");
   SHOW (int2mask (14), "zu");
   SHOW (int2mask (16), "zu");

   SHOW (int2msb (0), "zu");
   SHOW (int2msb (1), "zu");
   SHOW (int2msb (2), "zu");
   SHOW (int2msb (3), "zu");
   SHOW (int2msb (4), "zu");
   SHOW (int2msb (0x123), "#zu");
   SHOW (int2msb (0x223), "#zu");
   SHOW (int2msb (0x323), "#zu");
   SHOW (int2msb (0x489), "#zu");

   DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   SHOW (CONFIG_MAX_EVENTS_PER_PROCCESS, "zu");
   SHOW (sizeof (Event), "zu");
   SHOW (sizeof (EventBox), "zu");
   SHOW (sizeof (Process), "zu");
   SHOW (alignof (Event), "zu");
   SHOW (alignof (EventBox), "zu");
   SHOW (alignof (Process), "zu");
   SHOW (int2msb (Unfolding::PROC_SIZE), "zu");
   SHOW (int2mask (Unfolding::MAX_PROC - 1), "zu");
   SHOW (((size_t) u.proc(0)) >> (1 + int2msb (Unfolding::PROC_SIZE)), "x");

   SHOW (u.proc(0), "p");
   SHOW (u.proc(0) + 1, "p");
   SHOW (u.proc(1), "p");
   SHOW (u.proc(1) + 1, "p");
   SHOW (Unfolding::ptr2pid (u.proc(0)), "u");
   SHOW (Unfolding::ptr2pid (u.proc(0) + 1), "u");
   SHOW (Unfolding::ptr2pid (u.proc(1)), "u");
   SHOW (Unfolding::ptr2pid (u.proc(1) + 2), "u");

   DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   SHOW (int2mask (Unfolding::PROC_SIZE - 1), "zx");
   SHOW (Unfolding::ptr2pindex (u.proc(0) + 0), "u");
   SHOW (Unfolding::ptr2pindex (u.proc(1) + 0), "u");
   SHOW (Unfolding::ptr2pindex (u.proc(2) + 0), "u");
   SHOW (Unfolding::ptr2pindex (u.proc(3) + 0), "u");
   SHOW (Unfolding::ptr2pindex (u.proc(3) + 1), "u");
   SHOW (Unfolding::ptr2pindex (u.proc(3) + 2), "u");
   SHOW (Unfolding::ptr2pindex (u.proc(3) + 3), "u");

   DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   SHOW (u.proc(0)->offset (u.proc(0)), "zu");
   SHOW (u.proc(0)->offset (u.proc(0) + 1), "zu");
   SHOW (u.proc(0)->offset (u.proc(0) + 2), "zu");
   SHOW (u.proc(0)->offset (u.proc(0) + 10), "zu");
   SHOW (u.proc(0)->offset (u.proc(1)), "zu");

   DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   /*
    * Thread 0: start, creat, join, exit
    * Thread 1: start, exit
    */
   Event *e;

   // start
   e = u.event (0);
   ASSERT (e->flags.boxfirst);
   ASSERT (!e->flags.boxlast);
   ASSERT (e->action.type == ActionType::THSTART);
   ASSERT (e->pre_proc() == 0);
   ASSERT (e->pre_other() == 0);

   for (unsigned i = 0; i < u.num_procs(); i++)
   {
      Process *p = u.proc (i);
      DEBUG ("proc %p pid %d first-event %p", p, p->pid(), p->first_event());
      ASSERT (p->first_event() and p->first_event()->action.type == ActionType::THSTART);
      ASSERT (p->first_event());
      for (Event &e : *p)
      {
         DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
               &e, e.pid(), e.pre_proc(), e.pre_other(),
               e.flags.boxfirst ? 1 : 0,
               e.flags.boxlast ? 1 : 0,
               action_type_str (e.action.type));
      }
   }

   DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   for (unsigned i = 0; i < 5; i++)
   {
      DEBUG ("test: i %u proc %p", i, u.proc (i));
   }
}

void test28 ()
{
   Unfolding u;

   /*
    * Thread 0: start, creat, join, exit
    * Thread 1: start, exit
    */
   Event *es, *ec, *ej, *ex, *es1, *ex1;

   for (int i = 0; i < 2; i++)
   {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
      // start
      es = u.event (0);
      ASSERT (es->is_bottom ());
      ASSERT (es->flags.boxfirst);
      ASSERT (!es->flags.boxlast);
      ASSERT (es->action.type == ActionType::THSTART);
      ASSERT (es->pre_proc() == 0);
      ASSERT (es->pre_other() == 0);

      // creat
      ec = u.event ({.type = ActionType::THCREAT}, es);
      ASSERT (ec->pre_proc() == es);
      ASSERT (ec->pre_other() == 0);
      ASSERT (ec->action.type == ActionType::THCREAT);
      ASSERT (!ec->flags.boxlast and !ec->flags.boxfirst);
      ASSERT (ec->pre_proc()->post.size() == 1);
      ASSERT (ec->pre_proc()->post[0] = ec);

      // start in thread 1
      es1 = u.event (ec);
      ASSERT (es1->flags.boxfirst and !es1->flags.boxlast);
      ASSERT (es1->action.type == ActionType::THSTART);
      ASSERT (es1->pre_proc() == 0);
      ASSERT (es1->pre_other() == ec);

      // exit in thread 1
      ex1 = u.event ({.type = ActionType::THEXIT}, es1);
      ASSERT (!ex1->flags.boxfirst and !ex1->flags.boxlast);
      ASSERT (ex1->action.type == ActionType::THEXIT);
      ASSERT (ex1->pre_proc() == es1);
      ASSERT (ex1->pre_other() == 0);

      // join
      ej = u.event ({.type = ActionType::THJOIN}, ec, ex1);
      ASSERT (ej->pre_proc() == ec);
      ASSERT (ej->pre_other() == ex1);
      ASSERT (ej->action.type == ActionType::THJOIN);
      ASSERT (!ej->flags.boxlast and !ej->flags.boxfirst);
      ASSERT (ej->pre_proc()->post.size() == 2);
      ASSERT (ej->pre_proc()->post[0] = es1);
      ASSERT (ej->pre_proc()->post[1] = ej);

      // exit
      ex = u.event ({.type = ActionType::THEXIT}, ej);
      ASSERT (!ex->flags.boxfirst);
      ASSERT (ex->action.type == ActionType::THEXIT);
      ASSERT (ex->pre_proc() == ej);
      ASSERT (ex->pre_other() == 0);
      ASSERT (ex->pre_proc()->post.size() == 1);
      ASSERT (ex->pre_proc()->post[0] = ex);

      u.dump ();
   }
}

void test30()
{
   Event *es, *ec, *el, *eu, *ej, *ex, *es1, *ex1, *el1, *eu1 ;
   Unfolding u;

   /*
    * Th 0           Th 1
    * -------------- ---------------
    * start    
    * creat
    * lock 0x100
    * unlock 0x100
    *                start
    *                lock 0x100
    *                unlock 0x100
    *                exit
    * join
    * exit
    */

   // start
   es = u.event (nullptr); // bottom

   // creat
   ec = u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // start in thread 1
   es1 = u.event (ec);
   // H: START must be immediately created after its process creation?
   // C: Yess!! This is a limitation of my current algorithms

   // lock
   el = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

   //unlock
   eu = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // lock
   el1 = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, eu);

   //unlock
   eu1 = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

   // exit in thread 1
   ex1 = u.event ({.type = ActionType::THEXIT}, eu1);

   // join
   ej = u.event ({.type = ActionType::THJOIN, .val = 1}, eu, ex1);

   // exit
   ex = u.event ({.type = ActionType::THEXIT}, ej);

//   printf("ex.vclock: ");
//   ex->vclock.print();

   u.dump ();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   Cut c(u);
#if 0
   c.add (es);
   c.add (ec);
   c.add (el);
   c.add (eu);
   c.add (es1);
   c.add (el1);
   c.add (eu1);
   c.add (ex1);
   c.add (ej);
   c.add (ex);
   c.dump ();
#endif

#if 1
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
   c.clear ();
   c.add (es);
   c.add (ec);
   c.add (el);
   c.add (eu);
   c.dump();
#endif

   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
   std::vector<int> replay;
   cut_to_replay(u, c, replay);
   DEBUG_("Replay: ");
   for (unsigned i = 0; i < replay.size(); i++)
   {
      DEBUG_ ("%d ", replay[i]);
      if (i % 2 == 1) DEBUG_ (" ");
   }
   DEBUG ("");
}
//-----------------
void test31()
{

//   Vclock v1(0,1), v2(1,2), v3(2,4);
//   Vclock v4(v1,v2), v5(v2,v3);
//   Vclock v6(v4,v5);
//
//   printf("v1:\n");
//   v1.print();
//
//   printf("v2:\n");
//   v2.print();
//
//   printf("v3:\n");
//   v3.print();
//
//   printf("v4 = v1 + v2:\n");
//   v4.print();
//
//   printf("v5 = v2 + v3:\n");
//   v5.print();
//
//   printf("v6 = v4 + v5:\n");
//   v6.print();
//
//   // compare
//   if (v5 == v6)
//      printf("v5 = v6 \n");
//   else
//      if (v5 < v6)
//         printf("v5 < v6 \n");
//      else
//         if (v5 > v6)
//            printf("v5 > v6 \n");
//         else
//            printf("We can't compare them \n");
//
//   if (v6 > v5)
//      printf("v6 > v5\n");
//
//   if (v4 == v5)
//         printf("v4 = v5 \n");
//      else
//         if (v4 < v5)
//            printf("v4 < v5 \n");
//         else
//            if (v4 > v5)
//               printf("v4 > v5 \n");
//            else
//               printf("We can't compare them \n");
//
//
//   if (v1 < v4)
//      printf("v1 < v4\n");
//   else
//      printf("can't compare\n");
}

void test32()
{
   Event *es, *ec, *el, *eu, *ej, *ex, *es1, *ex1 ;
   Unfolding u;

   /*
    * Thread 0: start, creat, join, exit
    * Thread 1: start, exit
    */

   // start
   es = u.event (nullptr); // bottom

   // creat
   ec = u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // start in thread 1 (immediately after the creat ;)
   es1 = u.event (ec);

   // exit in thread 1
   ex1 = u.event ({.type = ActionType::THEXIT}, es1);

   // join
   ej = u.event ({.type = ActionType::THJOIN, .val = 1}, ec, ex1);

   // lock
   el = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ej, nullptr);

   //unlock
   eu = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // exit
   ex = u.event ({.type = ActionType::THEXIT}, eu);

//   printf("ex.vclock: ");
//   ex->vclock.print();

   u.dump ();
   u.print_dot();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   Config c(u);
   c.dump ();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   c.add (es);
   c.add (ec);
   c.add (es1);
   c.add (ex1);
   c.add (ej);
   c.dump ();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   c.add (el);
   c.add (eu);

   c.add (ex);
   c.dump ();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   c.clear ();
   c.add (es);
   c.dump();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   DEBUG("%p.cut:", ex1);
   ex1->cut.dump();
   DEBUG("%p.cut:", ex);
   ex->cut.dump();

   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
   if (ex1->is_pred_of(ex))
      DEBUG("%p is pred of %p", ex, ex1);
   else
      DEBUG("%p is not pred of %p", ex, ex1);

   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
   if (el->in_cfl_with(ex))
      DEBUG("%p is in cfl with %p", ex, el);
   else
      DEBUG("%p is not in cfl with %p", ex, el);
}

//-----------------------
void test33()
{
   DEBUG("Test compute_cex");
   Unfolding u;

   /*
    * Thread 0: start, creat, join, exit
    * Thread 1: start, exit
    */

   Event *es, *ec, *el, *eu, *ell, *euu, *ej, *ex; // Process 0
   Event *es1, *ex1, *el1,*eu1, *ell1, *euu1 ; // Process 1

   // start
   es = u.event (nullptr); // bottom

   // creat proc 1
   ec = u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // start in thread 1
   es1 = u.event (ec);

   // lock
   el = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

   //unlock
   eu = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // lock
   ell = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu, eu);

   //unlock
   euu = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell, ell);


   /// Process 1
   // lock
   el1 = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, euu);

   //unlock
   eu1 = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

   // lock
   ell1 = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu1, eu1);

   //unlock
   euu1 = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell1, ell1);

   //exit in thread 1
   ex1 = u.event ({.type = ActionType::THEXIT}, euu1);


   // Process 0
   // join
   ej = u.event ({.type = ActionType::THJOIN, .val = 1}, euu, ex1);

   // exit
   ex = u.event ({.type = ActionType::THEXIT}, ej);

   Config c(u);
   c.add (es);
   c.add (ec);
   c.add (el);
   c.add (eu);
   c.add (ell);
   c.add (euu);
   c.add (es1);
   c.add (el1);
   c.add (eu1);
   c.add (ell1);
   c.add (euu1);
   c.add (ex1);
   c.add (ej);
   c.add (ex);
   c.dump();

   u.dump();
   printf("\nxxxxxxxxxxxxxxxxxxxx");
//   compute_cex(u,c);
   u.dump();
   u.print_dot();

   for (auto & e : *u.proc(1))
   {
      DEBUG("%s", action_type_str(e.action.type));
      if (e.in_cfl_with(ell))
         DEBUG("%p is in cfl with %p", ell, &e);
      else
         DEBUG("%p is not in cfl with %p", ell, &e);
   }

}

void test34()
{
   Unfolding u;

     /*
      * Thread 0: start, creat, join, exit
      * Thread 1: start, exit
      */

     Event *es, *ec, *el, *eu, *ell, *euu, *ej, *ex; // Process 0
     Event *es1, *ex1, *el1,*eu1, *ell1, *euu1 ; // Process 1

     // start
     es = u.event (nullptr); // bottom

     // creat proc 1
     ec = u.event ({.type = ActionType::THCREAT, .val = 1}, es);

     // start in thread 1 (immediately after the corresponding creat)
     es1 = u.event (ec);

     // lock
     el = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

     //unlock
     eu = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

     // lock
     ell = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu, eu);

     //unlock
     euu = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell, ell);


     /// Process 1
     // lock
     el1 = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, euu);

     //unlock
     eu1 = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

     // lock
     ell1 = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu1, eu1);

     //unlock
     euu1 = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell1, ell1);

     //exit in thread 1
     ex1 = u.event ({.type = ActionType::THEXIT}, euu1);


     // Process 0
     // join
     ej = u.event ({.type = ActionType::THJOIN, .val = 1}, euu, ex1);

     // exit
     ex = u.event ({.type = ActionType::THEXIT}, ej);

     u.dump();

//     // check causality
//     if (ell->is_pred_of(euu1))
//        DEBUG("el1 is a pred of eu1");
//     else
//        DEBUG("not a predecessor");


//     /// check conflict in the same tree
//     if (check_cfl_same_tree(0,*ell,*euu1))
//        DEBUG("in conflict");
//     else
//        DEBUG("not in conflict");

     /// check conflict in the same tree
     if (ell1->in_cfl_with(euu1))
        DEBUG("in conflict");
     else
        DEBUG("not in conflict");
}

void test35 ()
{
   // main3:
   // 0 5  1 5  2 2  3 2  0 4  -1  -> standard (p0 creates p1,p2; p1 creates p3)
   // 0 2  1 3  -1 -> alternative (p0 creates p1,p3; p1 creates p2)
   std::vector<const char *> argv {"prog", "main3"};
   std::vector<int> rep;
   std::vector<int> replay0 {-1};
   std::vector<int> replay1 {0, 2,  1, 3,  -1};

   try
   {
      C15unfolder unf;

      // load code and set argv
      unf.load_bytecode ("./input.ll");
      unf.set_args (argv);
      
      // run the system 1 time
      Config c (unf.add_one_run (replay0));
      c.dump ();

      // compute the replay of that conf
      rep.clear ();
      cut_to_replay (unf.u, c, rep);
      DEBUG_("Replay: ");
      for (unsigned i = 0; i < rep.size(); i++)
      {
         DEBUG_ ("%d ", rep[i]);
         if (i % 2 == 1) DEBUG_ (" ");
      }
      DEBUG ("");
      DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

      // run the system one more time with a different replay
      c = unf.add_one_run (replay1);
      c.dump ();

      // compute the replay of that conf
      rep.clear ();
      cut_to_replay (unf.u, c, rep);
      DEBUG_("Replay: ");
      for (unsigned i = 0; i < rep.size(); i++)
      {
         DEBUG_ ("%d ", rep[i]);
         if (i % 2 == 1) DEBUG_ (" ");
      }
      DEBUG ("");


   } catch (const std::exception &e) {
      DEBUG ("Test threw exception: %s", e.what ());
      DEBUG ("Aborting!");
   }
}

void test36 ()
{
   try
      {
         std::vector<int> replay {-1};
         C15unfolder unf;
         unf.load_bytecode ("./input.ll");

         Config c (unf.add_one_run (replay));
         c.dump ();
         unf.compute_cex(unf.u, c);

      } catch (const std::exception &e) {
         DEBUG ("Test threw exception: %s", e.what ());
         DEBUG ("Aborting!");
      }

}

void test37 ()
{
}

void test38 ()
{
}

void test39 ()
{
}

