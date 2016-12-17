
==================================
DPU: Distributed Program Unfolding
==================================

Under development.

TODO notes
==========

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
- write icfl
- fix is_pred_of
- fix in_cfl_with
- write trail
- write the Disset

Huyen:

x vclocks
x Unfolding::print_dot
x BaseConfig
x BaseConfig -> replay
x compute_cex (u, c)
x fix conf2replay, bug when c.max contains null pointers : fixed. 
- compute_alt (BaseConfig &c, const std::vector<Event*> d, 

Needs
=====
- in_dicfl_with() to initialize the comb
- 

Improvements
============

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
