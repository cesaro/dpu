
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <array>
#include <memory>
#include <algorithm>
#include <sys/stat.h>

#include "unfolder/c15unfolder.hh" // must be before verbosity.h
#include "unfolder/pidpool.hh"
#include "verbosity.h"
#include "misc.hh"
#include "pes/event.hh"
#include "pes/unfolding.hh"
#include "pes/process.hh"
#include "pes/eventbox.hh"
#include "pes/config.hh"
#include "pes/cut.hh"
#include "opts.hh"
#include "datarace-analysis.hh"

namespace dpu
{

#if 0
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
   SHOW (sizeof (Eventbox), "zu");
   SHOW (sizeof (Process), "zu");
   SHOW (alignof (Event), "zu");
   SHOW (alignof (Eventbox), "zu");
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
   SHOW (Unfolding::ptr2puid (u.proc(0) + 0), "u");
   SHOW (Unfolding::ptr2puid (u.proc(1) + 0), "u");
   SHOW (Unfolding::ptr2puid (u.proc(2) + 0), "u");
   SHOW (Unfolding::ptr2puid (u.proc(3) + 0), "u");
   SHOW (Unfolding::ptr2puid (u.proc(3) + 1), "u");
   SHOW (Unfolding::ptr2puid (u.proc(3) + 2), "u");
   SHOW (Unfolding::ptr2puid (u.proc(3) + 3), "u");

   DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   SHOW (int2mask (Unfolding::ALIGN - 1), "zx");
   SHOW (Unfolding::ptr2uid (u.proc(0) + 0), "x");
   SHOW (Unfolding::ptr2uid (u.proc(1) + 0), "x");
   SHOW (Unfolding::ptr2uid (u.proc(2) + 0), "x");
   SHOW (Unfolding::ptr2uid (u.proc(3) + 0), "x");
   SHOW (Unfolding::ptr2uid (u.proc(3) + 1), "x");
   SHOW (Unfolding::ptr2uid (u.proc(3) + 2), "x");
   SHOW (Unfolding::ptr2uid (u.proc(3) + 3), "x");

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
#ifdef VERB_LEVEL_DEBUG
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
#endif
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
      ASSERT (ec->pre_proc()->node[0].post.size() == 1);
      ASSERT (ec->pre_proc()->node[0].post[0] = ec);

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
      ASSERT (ej->pre_proc()->node[0].post.size() == 1);
      ASSERT (ej->pre_proc()->node[1].post.size() == 1);
      ASSERT (ej->pre_proc()->node[0].post[0] = ej);
      ASSERT (ej->pre_proc()->node[1].post[0] = es1);

      // exit
      ex = u.event ({.type = ActionType::THEXIT}, ej);
      ASSERT (!ex->flags.boxfirst);
      ASSERT (ex->action.type == ActionType::THEXIT);
      ASSERT (ex->pre_proc() == ej);
      ASSERT (ex->pre_other() == 0);
      ASSERT (ex->pre_proc()->node[0].post.size() == 1);
      ASSERT (ex->pre_proc()->node[0].post[0] = ex);

      u.dump ();
   }
}

void test30()
{
   Event *es, *ec, *el, *eu, *ej, *ex, *ell, *es1, *ex1, *el1, *eu1 ;
   Unfolding u;

   /*
    * Th 0           Th 1
    * -------------- ---------------
    * start    
    * creat
    * lock 0x100
    * unlock 0x100
    *                start
    *                lock 0x102
    *                unlock 0x102
    *                exit
    * join
    * lock 0x102
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
   //el1 = u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, eu);
   el1 = u.event ({.type = ActionType::MTXLOCK, .addr = 0x102}, es1, nullptr);

   //unlock
   //eu1 = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);
   eu1 = u.event ({.type = ActionType::MTXUNLK, .addr = 0x102}, el1, el1);

   // exit in thread 1
   ex1 = u.event ({.type = ActionType::THEXIT}, eu1);

   // join
   ej = u.event ({.type = ActionType::THJOIN, .val = 1}, eu, ex1);

   // lock
   ell = u.event ({.type = ActionType::MTXLOCK, .addr = 0x102}, ej, eu1);

   // exit
   ex = u.event ({.type = ActionType::THEXIT}, ell);

//   printf("ex.vclock: ");
//   ex->vclock.print();

   u.dump ();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   Cut c(u);
#if 0
   c.fire (es);
   c.fire (ec);
   c.fire (el);
   c.fire (eu);
   c.fire (es1);
   c.fire (el1);
   c.fire (eu1);
   c.fire (ex1);
   c.fire (ej);
   c.fire (ex);
   c.dump ();
#endif

#if 1
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
   c.clear ();
   c.fire (es);
   c.fire (ec);
   c.fire (el);
   c.fire (eu);
   c.dump();
#endif

   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
   std::vector<int> replay;
   // cut_to_replay(u, c, replay); -> use C15unfolder::cut_to_replay
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
    * Th 0           Th 1
    * -------------- ---------------
    * start    
    * creat
    *                start
    *                exit
    * join
    * lock 0x100
    * unlock 0x100
    * exit
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

   // unlock
   eu = u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // exit
   ex = u.event ({.type = ActionType::THEXIT}, eu);

//   printf("ex.vclock: ");
//   ex->vclock.print();

   u.dump ();
   std::ofstream fs("dot/unf.dot", std::fstream::out);
   u.print_dot (fs);
   fs.close();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   Config c(u);
   c.dump ();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   c.fire (es);
   c.fire (ec);
   c.fire (es1);
   c.fire (ex1);
   c.fire (ej);
   c.dump ();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   c.fire (el);
   c.fire (eu);

   c.fire (ex);
   c.dump ();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   c.clear ();
   c.fire (es);
   c.dump();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   DEBUG("%p.cone:", ex1);
   ex1->cone.dump();
   DEBUG("%p.cone:", ex);
   ex->cone.dump();

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
   c.fire (es);
   c.fire (ec);
   c.fire (el);
   c.fire (eu);
   c.fire (ell);
   c.fire (euu);
   c.fire (es1);
   c.fire (el1);
   c.fire (eu1);
   c.fire (ell1);
   c.fire (euu1);
   c.fire (ex1);
   c.fire (ej);
   c.fire (ex);
   c.dump();

   u.dump();
   printf("\nxxxxxxxxxxxxxxxxxxxx");
   //compute_cex(u,c); -> use C15unfolder::compute_cex
   u.dump();
   std::ofstream fs("dot/unf.dot", std::fstream::out);
   u.print_dot (fs);
   fs.close();

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
      C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 0);

      // load code and set argv
      unf.load_bitcode ("./input.ll");
      unf.set_args (argv);
      
      // run the system 1 time
      Config c (unf.add_one_run (replay0));
      c.dump ();

      // compute the replay of that conf
      rep.clear ();
      unf.cut_to_replay (c, rep);
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
      unf.cut_to_replay (c, rep);
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
   DEBUG("test36");
   Event *e = nullptr;
   std::vector<const char *> argv {"hhghgjtuyen.c"};
   try
   {
      std::vector<int> replay {-1};
      C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);
      unf.load_bitcode ("./input.ll");
      unf.set_args (argv);

      Config c (unf.add_one_run (replay));
      c.dump ();
      unf.compute_cex(c, &e);

      // print dot
      std::ofstream f ("dot/huyen.dot");
      unf.u.print_dot (f);
      f.close ();

   } catch (const std::exception &e) {
      DEBUG ("Test threw exception: %s", e.what ());
      DEBUG ("Aborting!");
   }
}

void test37 ()
{
   DEBUG("Test compute_cex");
   C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);
   Event *e = nullptr;

   /*
    * Thread 0: start, creat, join, exit
    * Thread 1: start, exit
    */

   Event *es, *ec, *el, *eu, *ell, *euu, *ej, *ex; // Process 0
   Event *es1, *ex1, *el1,*eu1, *ell1, *euu1 ; // Process 1

   // start
   es = unf.u.event (nullptr); // bottom

   // creat proc 1
   ec = unf.u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // start in thread 1
   es1 = unf.u.event (ec);

   // lock
   el = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

   //unlock
   eu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // lock
   ell = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu, eu);

   //unlock
   euu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell, ell);


   /// Process 1
   // lock
   el1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, euu);

   //unlock
   eu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

   // lock
   ell1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu1, eu1);

   //unlock
   euu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell1, ell1);

   //exit in thread 1
   ex1 = unf.u.event ({.type = ActionType::THEXIT}, euu1);


   // Process 0
   // join
   ej = unf.u.event ({.type = ActionType::THJOIN, .val = 1}, euu, ex1);

   // exit
   ex = unf.u.event ({.type = ActionType::THEXIT}, ej);

   Config c(unf.u);
   c.fire (es);
   c.fire (ec);
   c.fire (el);
   c.fire (eu);
   c.fire (ell);
   c.fire (euu);
   c.fire (es1);
   c.fire (el1);
   c.fire (eu1);
   c.fire (ell1);
   c.fire (euu1);
   c.fire (ex1);
   c.fire (ej);
   c.fire (ex);
   c.dump();

   unf.u.dump();
   printf("\nxxxxxxxxxxxxxxxxxxxx");
   unf.compute_cex (c, &e);
   unf.u.dump();

   std::ofstream fs("dot/unf.dot", std::fstream::out);
   unf.u.print_dot (fs);
   fs.close();

   //-----Test find_alternative
   Config cc(unf.u);
   cc.fire (es);
   cc.fire (ec);
   cc.fire (el);
   cc.fire (eu);

   cc.dump();

   std::vector<Event *> d = {ell};

   Cut j (unf.u);

//   if (unf.find_alternative(cc,d,j,&e))
//   {
//      DEBUG("There is alternative J:");
//      j.dump();
//   }
//   else
//      DEBUG("No alternative");
}

void test38 ()
{
   DEBUG("Test find_alternative");
   C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);
   Event *e = nullptr;

   /*
    * Thread 0: start, creat, join, exit
    * Thread 1: start, exit
    */

   Event *es, *ec, *el, *eu, *ell, *euu, *ej, *ex; // Process 0
   Event *es1, *ex1, *el1,*eu1, *ell1, *euu1 ; // Process 1

   // start
   es = unf.u.event (nullptr); // bottom

   // creat proc 1
   ec = unf.u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // start in thread 1
   es1 = unf.u.event (ec);

   // lock
   el = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

   //unlock
   eu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // lock
   ell = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu, eu);

   //unlock
   euu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell, ell);


   /// Process 1
   // lock
   el1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, euu);

   //unlock
   eu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

   // lock
   ell1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu1, eu1);

   //unlock
   euu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell1, ell1);

   //exit in thread 1
   ex1 = unf.u.event ({.type = ActionType::THEXIT}, euu1);


   // Process 0
   // join
   ej = unf.u.event ({.type = ActionType::THJOIN, .val = 1}, euu, ex1);

   // exit
   ex = unf.u.event ({.type = ActionType::THEXIT}, ej);

   Config c(unf.u);
   c.fire (es);
   c.fire (ec);
   c.fire (el);
   c.fire (eu);
   c.fire (ell);
   c.fire (euu);
   c.fire (es1);
   c.fire (el1);
   c.fire (eu1);
   c.fire (ell1);
   c.fire (euu1);
   c.fire (ex1);
   c.fire (ej);
   c.fire (ex);
   c.dump();

   unf.u.dump();
   printf("\nxxxxxxxxxxxxxxxxxxxx");
   unf.compute_cex (c, &e);
   unf.u.dump();

   std::ofstream fs("dot/unf.dot", std::fstream::out);
   unf.u.print_dot (fs);
   fs.close();

   //-----Test find_alternative
   Config cc(unf.u);
   cc.fire (es);
   cc.fire (ec);
//   cc.fire (el);
//   cc.fire (eu);

   cc.dump();

   std::vector<Event *> d = {ell};

   Cut j (unf.u);

//   if (unf.find_alternative(cc,d,j,&e))
//   {
//      DEBUG("There is an alternative J:");
//      j.dump();
//   }
//   else
//      DEBUG("No alternative");
}

void test39 ()
{
   //test for find_alternative_only last
   DEBUG("Test find_alternative");
   C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);
   Event *e = nullptr;

   /*
    * Thread 0: start, creat, join, exit
    * Thread 1: start, exit
    */

   Event *es, *ec, *el, *eu, *ell, *euu, *ej, *ex; // Process 0
   Event *es1, *ex1, *el1,*eu1, *ell1, *euu1 ; // Process 1

   // start
   es = unf.u.event (nullptr); // bottom

   // creat proc 1
   ec = unf.u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // start in thread 1
   es1 = unf.u.event (ec);

   // lock
   el = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

   //unlock
   eu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // lock
   ell = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu, eu);

   //unlock
   euu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell, ell);


   /// Process 1
   // lock
   el1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, euu);

   //unlock
   eu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

   // lock
   ell1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu1, eu1);

   //unlock
   euu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell1, ell1);

   //exit in thread 1
   ex1 = unf.u.event ({.type = ActionType::THEXIT}, euu1);


   // Process 0
   // join
   ej = unf.u.event ({.type = ActionType::THJOIN, .val = 1}, euu, ex1);

   // exit
   ex = unf.u.event ({.type = ActionType::THEXIT}, ej);

   Config c(unf.u);
   c.fire (es);
   c.fire (ec);
   c.fire (el);
   c.fire (eu);
   c.fire (ell);
   c.fire (euu);
   c.fire (es1);
   c.fire (el1);
   c.fire (eu1);
   c.fire (ell1);
   c.fire (euu1);
   c.fire (ex1);
   c.fire (ej);
   c.fire (ex);
   c.dump();

   unf.u.dump();
   printf("\nxxxxxxxxxxxxxxxxxxxx");
   unf.compute_cex (c, &e);
   unf.u.dump();

   std::ofstream fs("dot/huyen.dot", std::fstream::out);
   unf.u.print_dot (fs);
   fs.close();

   //-----Test find_alternative
   Config cc(unf.u);
   cc.fire (es);
   cc.fire (ec);
   cc.fire (el);
   cc.fire (eu);

   cc.dump();

   Disset d;
   d.add (ell, 0);

   Cut j (unf.u);

   if (unf.find_alternative_only_last(cc,d,j))
   {
      DEBUG("There is an alternative J:");
      j.dump();
   }
   else
      DEBUG("No alternative");
}

void test40 ()
{
   std::vector<int> v = {1,2,3,4,5,6,7,8,9};
   // Dung auto
   for (auto &i : v)
   {
      if (i== 5)
      {
         i = v.back();
         v.pop_back();
         i = i * 10;
      }
      else
         i = i * 10;
   }

   //for (auto & i : v) DEBUG("%d ", i);
   // Dung while
//   int i = 0;
//   while (i < v.size())
//   {
//      if (v[i] == 5)
//      {
//         v[i] = v.back();
//         v.pop_back();
//         v[i] *= 10;
//      }
//      else
//      {
//         v[i] *= 10;
//         i++;
//      }
//   }
//
//   for (auto & i : v)
//      DEBUG("%d ", i);

}
void test41 ()
{
   //test for find_alternative_only last
      DEBUG("Test find_alternative");
      C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);
      Event *e = nullptr;

      /*
       * Thread 0: start, creat, join, exit
       * Thread 1: start, exit
       */

      Event *es, *ec, *el, *eu, *ell, *euu, *ej, *ex; // Process 0
      Event *es1, *ex1, *el1,*eu1, *ell1, *euu1 ; // Process 1

      // start
      es = unf.u.event (nullptr); // bottom

      // creat proc 1
      ec = unf.u.event ({.type = ActionType::THCREAT, .val = 1}, es);

      // start in thread 1
      es1 = unf.u.event (ec);

      // lock
      el = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

      //unlock
      eu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

      // lock
      ell = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu, eu);

      //unlock
      euu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell, ell);


      /// Process 1
      // lock
      el1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, euu);

      //unlock
      eu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

      // lock
      ell1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, eu1, eu1);

      //unlock
      euu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, ell1, ell1);

      //exit in thread 1
      ex1 = unf.u.event ({.type = ActionType::THEXIT}, euu1);


      // Process 0
      // join
      ej = unf.u.event ({.type = ActionType::THJOIN, .val = 1}, euu, ex1);

      // exit
      ex = unf.u.event ({.type = ActionType::THEXIT}, ej);

      Config c(unf.u);
      c.fire (es);
      c.fire (ec);
      c.fire (el);
      c.fire (eu);
      c.fire (ell);
      c.fire (euu);
      c.fire (es1);
      c.fire (el1);
      c.fire (eu1);
      c.fire (ell1);
      c.fire (euu1);
      c.fire (ex1);
      c.fire (ej);
      c.fire (ex);
      c.dump();

      unf.u.dump();
      printf("\nxxxxxxxxxxxxxxxxxxxx");
      unf.compute_cex (c, &e);
      unf.u.dump();

      std::ofstream fs("dot/huyen.dot", std::fstream::out);
      unf.u.print_dot (fs);
      fs.close();

      //-----Test find_alternative
      Config cc(unf.u);
      cc.fire (es);
      cc.fire (ec);
      cc.fire (el);
      cc.fire (eu);
      cc.fire (ell);
      cc.fire (euu);

      cc.dump();

//      Disset d;
//      d.add (ell, 0);

      std::vector<Event *> d;
      d.push_back(ell);
      Cut j (unf.u);

//      if (unf.find_alternative(cc,d,j)) // d is changed to Disset instead of std::vector
//      {
//         DEBUG("There is an alternative J:");
//         j.dump();
//      }
//      else
//         DEBUG("No alternative");
}
void test42 ()
{
   try
      {
         C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);

         // load code and set argv
         unf.load_bitcode ("./input.ll");
         unf.set_args ({"prog", "main4"});

         // build entire unfolding
         unf.explore ();

         // print dot
         unf.u.dump ();
         std::ofstream f ("dot/unf.dot");
         unf.u.print_dot (f);
         f.close ();

      } catch (const std::exception &e) {
         DEBUG ("Test threw exception: %s", e.what ());
         DEBUG ("Aborting!");
      }
}
void test43 ()
{
}
void test44 ()
{
}
void test45 ()
{
}
void test46 ()
{
}
void test47 ()
{
}
void test48 ()
{
}
void test49 ()
{
}

void test50 ()
{
#if 0
   // p0 creates p1,p2; p1 creates p3
   // std::vector<int> replay {0, 5, 1, 5, 2, 2, 3, 2, 0, 4, -1};
   // p0 creates p1,p3; p1 creates p2
   // std::vector<int> replay {0, 2, 1, 3, -1};
   std::vector<int> replay {-1}; // free mode
   std::vector<const char *> argv {"prog", "main3"};
#endif
#if 1
   // std::vector<int> replay {0, 2, 1, 2, -1}; // p1 lock first
   // std::vector<int> replay {0, 3, -1}; // p0 lock first
   std::vector<int> replay {-1}; // free mode
   std::vector<const char *> argv {"prog", "main4"};
#endif

   try
   {
      C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);

      // load code and set argv
      unf.load_bitcode ("./input.ll");
      unf.set_args (argv);
      
      // run system, get config, compute cex, and run 1 time per cex
      unf.add_multiple_runs (replay);
      //unf.add_one_run (replay);

      // print dot
      unf.u.dump ();
      std::ofstream f ("dot/unf.dot");
      unf.u.print_dot (f);
      f.close ();

   } catch (const std::exception &e) {
      DEBUG ("Test threw exception: %s", e.what ());
      DEBUG ("Aborting!");
   }
}

void test51 ()
{
   std::vector<const char *> argv {"prog", "main3"};
   try
   {
      Event *e = nullptr;
      std::vector<int> replay {-1};
      C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);
      unf.load_bitcode ("./input.ll");
      unf.set_args (argv);

      Config c (unf.add_one_run (replay));
      //Config cc (unf.add_one_run (replay));
      c.dump ();
      unf.compute_cex (c, &e);

      // dump dot for the unfolding
      std::ofstream f ("dot/unf.dot");
      unf.u.print_dot (f);
      f.close ();

      // dump on stdout
      unf.u.dump ();

   } catch (const std::exception &e) {
      DEBUG ("Test threw exception: %s", e.what ());
      DEBUG ("Aborting!");
   }
}

void test52 ()
{
   unsigned i;
   try
   {
      Event *e = nullptr, *ee;

      std::vector<int> replay {-1};
      C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);
      unf.load_bitcode ("./input.ll");
      unf.set_args ({"prog", "main4"});
      
      // run 1 time, compute cex
      Config c (unf.add_one_run (replay));
      c.dump ();
      unf.compute_cex (c, &e);

      // add one run per cex
      DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      unf.add_multiple_runs (replay);
      DEBUG ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

      // dump to stdout and in dot format
      unf.u.dump ();
      std::ofstream f ("dot/unf.dot");
      unf.u.print_dot (f);
      f.close ();

      ASSERT (c[0]->action.type == ActionType::THEXIT);
      SHOW (c[0]->str().c_str(), "s");

      // compute the find_pred<0> (i) for i >= 0 for all events of process 0
      for (e = c[0]; e; e = e->pre_proc ())
      {
         for (unsigned i = 0; i < e->depth_proc(); i++)
         {
            ee = e->node[0].find_pred<0>(i);
            ASSERT (ee);
            DEBUG ("d %d %s", i, ee->str().c_str());

            // and printing its immediate conflicts
            DEBUG (" icfls %d", ee->icfls().size());
#ifdef VERB_LEVEL_DEBUG
            for (Event *eee : ee->icfls ())
            {
               DEBUG (" icfl %s", eee->str().c_str());
            }
#endif
         }
         DEBUG ("xxxxxxxxxxxxxx");
      }

      // n^2 causality / conflict tests
      for (i = 0; i < unf.u.num_procs(); i++)
      {
#ifdef VERB_LEVEL_DEBUG
         for (Event &e : *unf.u.proc(i))
         {
            for (unsigned j = 0; j < unf.u.num_procs(); j++)
            {
               for (Event &ee : *unf.u.proc(j))
               {
                  DEBUG ("==================");
                  DEBUG (" %s", e.str().c_str());
                  DEBUG (" %s", ee.str().c_str());
                  //DEBUG (" %d", e.is_pred_of (&ee));
                  DEBUG (" %d", e.in_cfl_with (&ee));
               }
            }
         }
#endif
      }

   } catch (const std::exception &e) {
      DEBUG ("Test threw exception: %s", e.what ());
      DEBUG ("Aborting!");
   }
}

void test53 ()
{
   Event *es, *ec, *el, *eu, *xx, *ej, *ex, *es1, *ex1, *el1, *eu1 ;
   C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);

   /*
    * Th 0           Th 1
    * -------------- ---------------
    * start    
    * creat
    *                start
    * lock 0x100
    * unlock 0x100
    * lock 0x102
    *                lock 0x100
    *                unlock 0x100
    *                exit
    * join
    * exit
    */

   // start
   es = unf.u.event (nullptr); // bottom

   // creat
   ec = unf.u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // start in thread 1
   es1 = unf.u.event (ec);

   // lock
   el = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

   // unlock
   eu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // lock
   xx = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x102}, eu, nullptr);

   // lock
   el1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, eu);

   // unlock
   eu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

   // exit in thread 1
   ex1 = unf.u.event ({.type = ActionType::THEXIT}, eu1);

   // join
   ej = unf.u.event ({.type = ActionType::THJOIN, .val = 1}, xx, ex1);

   // exit
   ex = unf.u.event ({.type = ActionType::THEXIT}, ej);

   unf.u.dump ();
   std::ofstream fs("dot/unf.dot", std::fstream::out);
   unf.u.print_dot (fs);
   fs.close();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   Cut c (5);
   Cut j (5);

   c.fire (es);
   c.fire (ec);
   c.fire (el);
   c.fire (eu);
   c.fire (xx);

   j.fire (es);
   j.fire (ec);
   j.fire (el);
   j.fire (eu);
   j.fire (es1);
   j.fire (el1);
   j.fire (eu1);

   c.dump ();
   j.dump ();

   std::vector<int> replay;
   unf.cut_to_replay (c, j, replay);
   DEBUG_("Replay: ");
   for (unsigned i = 0; i < replay.size(); i++)
   {
      DEBUG_ ("%d ", replay[i]);
      if (i % 2 == 1) DEBUG_ (" ");
   }
   DEBUG ("");
}

void test54 ()
{
   try
   {
      C15unfolder unf (C15unfolder::Alt_algorithm::OPTIMAL, 1);

      // load code and set argv
      unf.load_bitcode ("./input.ll");
      unf.set_args ({"prog", "main4"});
      
      // build entire unfolding
      unf.explore ();

      // print dot
      unf.u.dump ();
      std::ofstream f ("dot/unf.dot");
      unf.u.print_dot (f);
      f.close ();

   } catch (const std::exception &e) {
      DEBUG ("Test threw exception: %s", e.what ());
      DEBUG ("Aborting!");
   }
}

#endif

void test55 ()
{
   Event *es, *ec, *el, *eu, *ecc, *ex;
   Event *es1, *ex1, *el1, *eu1;
   Event *es2, *ex2;
   C15unfolder unf (Altalgo::OPTIMAL, 1, UINT_MAX);

   /*
    * Th 0           Th 1           Th2
    * -------------- -------------- -------------
    * Execution 1
    * ***********
    * start    
    * creat 1
    * lock 0x100
    * unlock 0x100
    * creat 2
    * exit
    *                start
    *                lock 0x100
    *                unlock 0x100
    *                exit
    *                               start
    *                               exit
    *
    * Execution 2
    * ***********
    * start    
    * creat 1
    *                start
    *                lock 0x100
    *                unlock 0x100
    *                exit
    * lock 0x100
    * unlock 0x100
    * creat 2
    *                               start
    *                               exit
    * exit
    */

   // Execution 1 ****************

   // start
   es = unf.u.event (nullptr); // bottom

   // creat
   ec = unf.u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // lock
   el = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, nullptr);

   // unlock
   eu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // creat
   ecc = unf.u.event ({.type = ActionType::THCREAT, .val = 2}, eu);

   // exit
   ex = unf.u.event ({.type = ActionType::THEXIT}, ecc);

   // start in thread 1
   es1 = unf.u.event (ec);

   // lock
   el1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, eu);

   // unlock
   eu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

   // exit in thread 1
   ex1 = unf.u.event ({.type = ActionType::THEXIT}, eu1);

   // start in thread 2
   es2 = unf.u.event (ecc);

   // exit in thread 2
   ex2 = unf.u.event ({.type = ActionType::THEXIT}, es2);

   Cut c (5);
   c.fire (es);
   c.fire (ec);
   c.fire (el);
   c.fire (eu);
   c.fire (ecc);
   c.fire (es2);
   c.fire (ex2);
   c.fire (es1);

   // Execution 2 ****************

   // start
   es = unf.u.event (nullptr); // bottom

   // creat
   ec = unf.u.event ({.type = ActionType::THCREAT, .val = 1}, es);

   // start in thread 1
   es1 = unf.u.event (ec);

   // lock
   el1 = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, es1, nullptr);

   // unlock
   eu1 = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el1, el1);

   // exit in thread 1
   ex1 = unf.u.event ({.type = ActionType::THEXIT}, eu1);

   // lock
   el = unf.u.event ({.type = ActionType::MTXLOCK, .addr = 0x100}, ec, eu1);

   // unlock
   eu = unf.u.event ({.type = ActionType::MTXUNLK, .addr = 0x100}, el, el);

   // creat
   ecc = unf.u.event ({.type = ActionType::THCREAT, .val = 2}, eu);

   // exit
   ex = unf.u.event ({.type = ActionType::THEXIT}, ecc);

   // start in thread 2
   es2 = unf.u.event (ecc);

   // exit in thread 2
   ex2 = unf.u.event ({.type = ActionType::THEXIT}, es2);

   unf.u.dump ();
   std::ofstream fs ("u.dot", std::fstream::out);
   unf.u.print_dot (fs);
   fs.close();
   printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

   DEBUG ("execution 1:");
   c.dump ();

   Cut j (5);
   j.fire (es);
   j.fire (ec);
   j.fire (es1);
   j.fire (el1);
   j.fire (eu1);
   j.fire (el);
   j.fire (eu);
   j.fire (ecc);
   j.fire (es2);
   j.fire (ex2);
   j.dump ();
}

void test56 ()
{
   Unfolding u;
   Pidpool p (u);
   p.dump ();
   u.dump ();
}

void test57 ()
{
   stid::ExecutorConfig config;
   config.strace.pthreads = 1;
   config.do_load_store = 1;

   Unfolder<> unf (config);
   unf.load_bitcode ("/tmp/input.ll");
   unf.print_external_syms (nullptr);
   unf.set_default_environment ();
   unf.set_args ({"c", "arg1", "arg2"});

   DEBUG ("before running steroids:");
   unf.u.dump ();
   
   //// run 1 time in free mode
   //Replay replay1 (unf.u);
   //SHOW (replay1.str().c_str(), "s");
   //Config c1 (unf.add_one_run (replay1));
   //c1.dump ();

   // run 1 time in free mode
   Replay replay2 (unf.u, {0, 3, 2, 3, 1, 3});
   SHOW (replay2.str().c_str(), "s");
   Config c2 (unf.add_one_run (replay2));
   c2.dump ();

   // dump to stdout and in dot format
   unf.u.dump ();
   unf.u.print_dot ("u.dot");
}

void test58 ()
{

   char *argv[] = {(char*) "dpu", (char*) "/tmp/input.ll", (char*) "-vvv"};
   int argc = 3;

   // necessary so that the DataRaceAnalysis initializes itself correctly
   opts::parse (argc, argv);

   dpu::DataRaceAnalysis dra;
   dra.load_bitcode (std::string (opts::inpath));
   dra.print_external_syms (nullptr);
   dra.set_default_environment ();
   dra.set_args ({"p", "arg1", "arg2"});

   DEBUG ("before running steroids:");
   dra.u.dump ();

   // run 1 time in free mode
   Replay r1 (dra.u);
   Replay r2 (dra.u, {0, 3, 2, 3, 1, 3});
   Replay r3 (dra.u, {0, 3, 1, 1, 2, 2});
   std::vector<stid::Replay> v {r1, r2, r3}; // FIXME cast done here!

   dra.replays = v;
   dra.run ();

   SHOW (dra.race, "p");

   // dump unfolding to stdout and in dot format
   dra.u.dump ();
   dra.u.print_dot ("u.dot");

   // and also the report
   dra.report.save ("/tmp/defects.yml");
}

void test59 ()
{
}

} // namespace
