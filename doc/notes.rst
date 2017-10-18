
Tasks
=====

Cesar:
x escribir el ultimo Unfolding::event
x escribir los constructores de los Events
x escribir pre_{proc,other}
x compilar
x hacer un test
x escribir el iterador de eventos de un proceso
x limpiar el .hh
x check alignment Event / Event box
x write methods to retrieve existing events when replying
x how to give semantics to failing create/join
x escribir la funcion de busqeuda en los post para los Unfolding::event
x steroids -> unf
x C15unfolder::stream_to_events
x testing stream_to_events
x design unfolder class
x add_multiple_runs
x reimplement Node<T,SS>
x write in_icfl_with (Event e);
x how to represent conflict?
x write icfls()
x improved dot output
x fix is_pred_of
x fix bug in compute_cex
x fix in_cfl_with
x debug Primeconfig
x debug in_cfl_with
x write trail
x write the Disset
x Cut::unfire (e)
x Config::unfire (e)
x Disset::unadd()
x alt_to_replay
x modify stream_to_events to update the trail
x write explore
x write the driver script: c -> ll -> add runtime -> optimize -> verify
x Trail::dump(prefix) with only replay info
x find_alternative_optim_comb comb [1 7 30 4] prun 27 cq 130 no|found
x test on 10 svcomp bench
x test alt-optim()
x make visible the SSB counter
x optimize
x debug printing to understand when the alt() NP-hard explosion happens


Huyen:
x vclocks
x Unfolding::print_dot
x BaseConfig
x BaseConfig -> replay
x write the simplest possibly-incomplete method to compute alternatives
x In Event, store a Config instead of Cut as an event's local configuration.
  When deciding the conflict between two arbitrary events e and e', just look at its local
  configuration's mutexmax. Having e.mutexmax(m) !cfl e'.mutexmax(m) for every mutex variable m
  means that e !cfl with e'.
x write find_alternative
x adapt find_alternative to use the Disset instead of a std::vector


For a near future:

- print counterexample run (Config)
- find benchmarks
- warn about dataraces
- steroids: send back ERROR actions; capture calls to abort(3), __assert_fail,
  and friends


Future improvements
===================

- Better algorithm for finding pre-existing events in the unfolding when we
  instert new events, based on a radix tree or hash table per event; or at least
  optimize to scan the smallest postset in Unfolding::find2

x Vector clocks. In the current implementation, vclocks store a mapping from
  pids to integers. The integer increments whenever the process advances.
  Instead of storing integers I think we could store a pointer to the last event
  (this), as they are also well ordered in memory. An undersirable outcome of
  this is that sibling nodes of the per-process tree will *not* have equal
  depths, but depths that are now ordered, as well. This seems to have no impact
  if we are going to only use these clocks only for recovering causaly
  information inside of configurations.
  -> removed from Event thank to Cut existence.

x translate e->action.val on thread creation, so that it contains the pid in the
  unfolding, not the one in steroids

- design interface in Config to enumerate the addresses of mutexes involved in
  the configuration

- In the Node<> and MultiNode<> templates, we could remove the pointer pre, as
  our current datastructure for Events do not need to store it. We could just
  assume that the type T has a function T * T::get_pre() and call it.
  Mhh, not so clear how to do it...

- Optimize pointers stored in the Node<T,SS>::skiptab for depths equal to a
  power of SS, as the last pointe is systematically a pretty useless pointer to
  the root of the tree.

- Event::icfls() should return an InputIterator rather than an std::vector

- Primeconfig::lockmax should be a fixed-size vector, rather than std::vector

- Primeconfig::merge3_ways should be a template, to optimize for the 3rd parameter

- The underlying storage space for the Trail should only grow

- in alt_to_replay(), the computation of a replay sequence for C can be done
  with a linear scan of the trail, instead of C, and will be faster.

- in cut_to_replay(c1,c2,rep) we have a complex test to check when the 
  pre_other() of one event has already been visited; we could reuse the old idea
  of marking events with a color, but that would require having c1 already
  marked; we could do this if we add a new integer argument to the method and
  assume that c1 is marked with that color

- We reserve a fixed capacity for the vector Disset::stack to avoid rellocations
  that break pointers in the linked lists. This could be more nicely done.

- do a simple configure script, allowing to select for debug compilation
  (CONFIG_DEBUG, -g, no optimization) and release compilation (undef
  CONFIG_DEBUG, -O3, low verbosity level

- turn CONFIG_GUEST_MEMORY_SIZE and friends into commandline options

- prepare regression tests and a Makefile goal "tests" to run them

x Add an option ``-D MACRO`` to dpu

- We can make the maximum number of processes dynamic (commandline option) if we
  keep the max number of events per process static (define). To do it we
  mallocate memory for nprocs*evperproc*sizeof(Event) with the best alignment
  possile, using a trial and error process. If we manage to mallocate something
  suitable for that nprocs, then we are done. Otherwise we divide nprocs by 2
  and restart. We continue until we get it done, and inform the user if nprocs
  have been reduced. We could even remove the commandline option and let the
  tool start with a high nprocs and find automatically the maximum accepted on
  the machine.

- The spikes of the comb are built using the contents returned by
  Event::icfls(); on the benchmark mpat, for k=5, we have that 60% of the time
  spent in explore() is spent in find_alternatives(), and 61% of that time is
  spent in the function Event::icfls(); in average, icfls() builds spikes of
  size 156, and after filtering out undesired events, the spikes have 7.8
  events (plus 80% of them have either size 1, or 2, or 3; 20% beyond 3).
  This is an effect of not removing events from the overall structure.
  For k=6, the filtering is one order of magnitude stronger, we go from 1383
  events per unfiltered spike to 8.8 events after filtering.

- Modify steroids so that we receive THSTART actions rather than having to guess
  when it is the first context switch to that thread. The lack of actions makes
  the logic in stream_to_events rather complex.

- Add regression tests ensuring that the dot output is still working. Run dot(1)
  on the output to ensure it is good.

- The replay class has one annoying feature: it requires having the unfolding to
  optimize the size of the array scanned in one of the `extend_from` functions.
  This could be obtained from UnfoldingMemoryMath::MAX_PROC, but we can do
  better by taking the unfolding. A consequence of having to take the unfolding
  is the the object is not default constructible, which means that a Defect is
  not default constructible should it use the dpu::Replay instead of the
  stid::Replay, as it should. Similarly the DataRaceAnalysis cannot store a
  vector of dpu::Replay because it's not default constructible (grrrrrrrrr).

- include deadlocking programs in the regression tests

Alternatives
============

Incomplete methods
------------------

- perform a fast search; if you find one, return it
- if not, return that there is no alternative
- this is incomplete, there is nothing else to modify in the algorithm

Complete but unoptimal methods
------------------------------

- perform a fast search; if you find one, return it
- if not, but you find an immediate conflict of the last that qualifies (no
  conflicts with C), return it as an alternative, even if it is not
- when replaying it you might unavoidably try to add events in D to C (SSB)
- modify stream_to_events to refuse to do this, this configuration has
  necessarily already been explored
- nothing else to modify?

Complete and optimal methods
----------------------------

- the algorithm we have discussed


find_alternative_only_last
--------------------------

- (complete but unoptimal)
- consider the last event in D, call it e
- if you find some immediate conflict e' of e that is compatible with C (that
  is, e' is not in conflict with any event in proc-max(C)), then set J = [e']
  and return it
- if you don't find any such e', return false

