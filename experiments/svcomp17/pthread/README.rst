
Folder ``pthread/``
===================

This folder contains the benchmarks from the folder ``c/pthread/`` of the
SVCOMP'17. In this README we comment on

- the modifications performed to the source code that we had to do to anlayze
  them with DPU, and
- the results of the analysis with DPU.

File ``bigshot_p_false-unreach-call.c``
---------------------------------------

Changes to the source code:

- Remove warnings (add ``return`` statements)
- Remove data races using a single global mutex

DPU finds the bug in less than 1 second.


Files ``bigshot_s{,2}_true-unreach-call.c``
---------------------------------------

Changes to the source code:

- Remove warnings (add ``return`` statements)
- No need to remove data races because there is no concurrency at all!

These benchmarks have only 1 Mazurkiewicz trace.
DPU proves safety in less than 1 second.


File ``fib_bench_false-unreach-call.c``
---------------------------------------

Changes to the source code:

- Addeed one global mutex and one critical section on each thread loop and
  around the conditional in the ``main`` function.

DPU finds 2772 Mazurkiewicz traces, two of which expose the bug, and finished
the exploration in 0.36 seconds::

 $ dpu fib_bench_false-unreach-call.c  | egrep 'VERIFIER|unfolding|summary'
 dpu: __VERIFIER_error called, calling abort()
 dpu: __VERIFIER_error called, calling abort()
 dpu: por: stats: unfolding: 2772 max-configs
 dpu: por: stats: unfolding: 3 threads created
 dpu: por: stats: unfolding: 3 process slots used
 dpu: por: stats: unfolding: 25058 events (aprox. 5M of memory)
 dpu: por: stats: unfolding: t0: 2772 events (570K, 11.1%)
 dpu: por: stats: unfolding: t1: 11143 events (2M, 44.4%)
 dpu: por: stats: unfolding: t2: 11143 events (2M, 44.6%)
 dpu: por: summary: 2 defects, 2772 max-configs, 0 SSBs, 25058 events, 0.362 sec, 25M


Files ``fib_bench_longer_{false,true}-unreach-call.c``
-----------------------------------------------------

Changes to the source code:

- Same as in the previous ``fib`` benchmark.

DPU explores 12012 Mazurkiewicz in 1.2 seconds or less, and finds that two of them
expose the bug in the ``false`` benchmark. It also proves safe the ``true``
benchmark::

 $ dpu fib_bench_longer_false-unreach-call.c  | grep summary
 dpu: por: summary: 2 defects, 12012 max-configs, 0 SSBs, 109958 events, 1.218 sec, 53M

 $ dpu fib_bench_longer_true-unreach-call.c  | grep summary
 dpu: por: summary: 0 defects, 12012 max-configs, 0 SSBs, 109958 events, 1.031 sec, 52M


Files ``fib_bench_longest_{false,true}-unreach-call.c``
-------------------------------------------------------

Changes to the source code:

- Same as in the previous ``fib`` benchmark.

In the unsafe benchmark, DPU is unable to find to find the bug in less than 30s,
after having explored 434K Mazurkiewicz traces. It is also unable to find the
bug if the number of context switches is limited to 6 or 7 (note that the number
of defects is ``0``)::

 $ dpu fib_bench_longest_false-unreach-call.c  --timeout 30 | grep summary
 dpu: por: summary: 0 defects, 434220 max-configs, 0 SSBs, 4194916 events, 30.823 sec, 1359M (timeout

 $ dpu fib_bench_longest_false-unreach-call.c -k0 -x7 | grep summary
 dpu: por: summary: 0 defects, 103311 max-configs, 0 SSBs, 1939605 events, 75.429 sec, 642M

In the safe benchmark, DPU can prove that no bug is reachable in up to 7 context
switches in 71 seconds::

 $ dpu fib_bench_longest_true-unreach-call.c  -O3 -k0 -x7 | egrep 'unfolding|summary'
 dpu: por: stats: unfolding: 103311 max-configs
 dpu: por: stats: unfolding: 3 threads created
 dpu: por: stats: unfolding: 3 process slots used
 dpu: por: stats: unfolding: 1939605 events (aprox. 388M of memory)
 dpu: por: stats: unfolding: t0: 117483 events (23M, 6.1%)
 dpu: por: stats: unfolding: t1: 849296 events (169M, 43.6%)
 dpu: por: stats: unfolding: t2: 972826 events (195M, 50.3%)
 dpu: por: summary: 0 defects, 103311 max-configs, 0 SSBs, 1939605 events, 70.648 sec, 646M


File ``sigma_false-unreach-call.c``
-----------------------------------

Changes to the source code:

- Removed data races on the accesses to the ``array`` and the ``array_index``.
- Value of ``SIGMA`` reduced from 16 to 5. DPU will find the bug almost
  immediately, but the tool currently doesn't stop at the first bug found and
  will explore all exponentially many Mazurkiewicz traces.

File ``singleton_false-unreach-call.c``
---------------------------------------

Changes to the source code:

- Removed data races

DPU finds the bug in less than 1 second.

File ``singleton_with-uninit-problems_true-unreach-call.c``
-----------------------------------------------------------

Changes to the source code:

- Removed data races

DPU proves safety in less than 1 second.

Files ``stack{,_longer,_longest}_false-unreach-call.c``
-----------------------------------------------------

Changes to the source code:

- Removed a couple of compilation warnings (missing ``return`` statements).

DPU finds the error but immediatly after it graciously crashes with an error
message, explaining that it does not support calls to ``exit`` or ``abort`` made
from threads other than the main thread::

 $ dpu stack_longest_false-unreach-call.c
 clang-3.7 -I /x/home/polaris/local/bin/../include -D__DPU__ -O3 -emit-llvm -c -o /tmp/dpu.lwlV.stack_longest_false-unreach-call.c.opt.bc -- 'stack_longest_false-unreach-call.c'
 llvm-link-3.7 /tmp/dpu.lwlV.stack_longest_false-unreach-call.c.opt.bc /x/home/polaris/local/bin/../lib/dpu/rt.bc -o /tmp/dpu.lwlV.stack_longest_false-unreach-call.c.bc
 /x/home/polaris/local/bin/../lib/dpu/dpu-backend /tmp/dpu.lwlV.stack_longest_false-unreach-call.c.bc
 dpu v0.5.0 running, pid 2940
 dpu: performing the following analyses:
 dpu: - POR: checking for assertion violations + deadlocks
 dpu: por: using 'only-last' (1-partial) alternatives
 dpu: unf: loading bitcode
 dpu: unf: O1-optimization + jitting...
 dpu: por: starting POR analysis ...
 stack underflow
 dpu: __VERIFIER_error called, calling abort()
 rt/libc/proc.c:29: _rt_abort: Thread 2: Function abort() was called, but current support for abort() is very limited
 rt/libc/proc.c:32: _rt_abort: Thread 2: calling pthread_exit() instead of exit(), you might experience unexpected errors from this point on
 rt/thread-sched.c:153: __rt_thread_sched_find_any: deadlock found: no thread is runnable, empty sleep set
 rt/thread-sched.c:327: __rt_thread_term: error: thread 2 called exit() but this runtime only supports calls to exit() from the main thread

Files ``stack{,_longer,_longest}_true-unreach-call.c``
-----------------------------------------------------

Changes to the source code:

- Removed a couple of compilation warnings (missing ``return`` statements).

For ``stack_true`` DPU proves safety without any problems in 0.2 seconds. There
are only 6 Mazurkiewicz traces to consider::

 $ dpu stack_true-unreach-call.c -k0 -x2 | grep summary
 dpu: por: summary: 0 defects, 6 max-configs, 0 SSBs, 150 events, 0.221 sec, 20M

In the ``longer`` and ``longest`` benchmarks, DPU proves safety in a reasonable
time only when bounding the number of context switches to 2::

 $ dpu stack_longest_true-unreach-call.c -k0 -x2 | egrep 'unfolding|summary'
 dpu: por: stats: unfolding: 801 max-configs
 dpu: por: stats: unfolding: 3 threads created
 dpu: por: stats: unfolding: 3 process slots used
 dpu: por: stats: unfolding: 2248410 events (aprox. 448M of memory)
 dpu: por: stats: unfolding: t0: 2406 events (495K, 0.1%)
 dpu: por: stats: unfolding: t1: 643202 events (127M, 28.5%)
 dpu: por: stats: unfolding: t2: 1602802 events (320M, 71.4%)
 dpu: por: summary: 0 defects, 801 max-configs, 0 SSBs, 2248410 events, 1.711 sec, 762M


Files ``stateful01_{true,false}-unreach-call.c``
------------------------------------------------

Changes to the source code:

- Removed a couple of compilation warnings (missing ``return`` statements).

DPU finds the bug in the ``false`` benchmark and proves safety in the ``true``
benchmark in less than 1 second.

File ``sync01_true-unreach-call.c``
-----------------------------------

DPU cannot analyze this benchmark because it uses conditional variables and DPU
does not support them.

A seeminly trivial solution could be to implement the functions
``pthread_cond_{init,wait,signal}`` as follows:

.. code:: c

 void my_pthread_cond_init (pthread_cond_t * cond, pthread_condattr_t * attr)
 {
 }
 
 void my_pthread_cond_wait (pthread_cond_t * cond, pthread_mutex_t * m)
 {
    pthread_mutex_unlock (m);
    pthread_exit (0);
 }
 
 void my_pthread_cond_signal (pthread_cond_t * cond)
 {
 }

However, this solution fails, because in one of the interleavings the thread
(``thread1``) exits instead of waiting until the conditional variable is
signalled, thus missing the increment of ``num`` and failing the assertion in
the ``main`` function.


File ``twostage_3_false-unreach-call.c``
----------------------------------------

The benchmark has a data race, which DPU can detect and print as follows (the
``Offending execution`` is, quite primitively, represented as a partial order)::

 $ dpu twostage_3_false-unreach-call.c -a dr
 [...]
 dpu: data race analysis requested, reloading source...
 dpu: unf: loading bitcode
 dpu: unf: O1-optimization + jitting...
 dpu: dr: starting data-race detection analysis on 1 executions...
 dpu: dr: finished data-race detection
 dpu: dr: result: one data race FOUND (but the program may have more)
 == begin data race ==
  Data race: threads 1 and 2 can concurrently access and modify variable 0x7f37c7ce1040 (1 bytes)
 
  Offending execution:
 == begin cut ==
 Proc 0
  e 0000001c0 p  0 d 02,02,00 pre 000000f0 00000000 ---- dat 1+0 ac THCREAT val 2
  e 0000000f0 p  0 d 01,01,00 pre 00000020 00000000 ---- dat 1+0 ac THCREAT val 1
  e 000000020 p  0 d 00,00,00 pre 00000000 00000000 f--- dat 2+1 ac THSTART
 Proc 1
  e 020000290 p  1 d 05,03,00 pre 200001c0 00000000 ---- dat 2+1 ac MTX-LOCK addr 0x7f37c7ceb4c0
  e 0200001c0 p  1 d 04,02,01 pre 200000f0 200000f0 ---- dat 1+0 ac MTX-UNLK addr 0x7f37c7ceb490
  e 0200000f0 p  1 d 03,01,00 pre 20000020 00000000 ---- dat 1+1 ac MTX-LOCK addr 0x7f37c7ceb490
  e 020000020 p  1 d 02,00,01 pre 00000000 000000f0 f--- dat 1+0 ac THSTART
 Proc 2
  e 0400000f0 p  2 d 05,01,02 pre 40000020 200001c0 ---- dat 1+1 ac MTX-LOCK addr 0x7f37c7ceb490
  e 040000020 p  2 d 03,00,01 pre 00000000 000001c0 f--- dat 1+0 ac THSTART
 == end cut ==
 == end data race ==
 dpu: saving data-race defects report to 'defects.dr.yml'
 [...]

DPU finds the bug without modifications of the source::

 $ dpu twostage_3_false-unreach-call.c 2>&1 | grep 'Bug found' | wc -l
 8

To be Processed
---------------

The following benchmarks can be easily adapted (when necessary, e.g., to remove
data races) to run with DPU::

 indexer_true-unreach-call.c
 lazy01_false-unreach-call.c
 queue_false-unreach-call.c
 queue_longer_false-unreach-call.c
 queue_longest_false-unreach-call.c
 queue_ok_longer_true-unreach-call.c
 queue_ok_longest_true-unreach-call.c
 queue_ok_true-unreach-call.c
 reorder_2_false-unreach-call.c
 reorder_5_false-unreach-call.c

