
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
- how to represent conflict?
- steroids -> unf
- design unfolder class

Huyen:

x vclocks
x Unfolding::print_dot
- BaseConfig
- BaseConfig -> replay
- compute_cex (u, c)
- fix conf2replay, bug when c.max contains null pointers : fixed. 
==========
Questions:
THSTART event must be created immediately after the THCREAT?
If not, there is a bug.

Improvements
============

- Better algorithm for finding pre-existing events in the unfolding when we
  instert new events, based on a radix tree or hash table per event; or at least
  optimize to scan the smallest postset in Unfolding::find2

- Vector clocks. In the current implementation, vclocks store a mapping from
  pids to integers. The integer increments whenever the process advances.
  Instead of storing integers I think we could store a pointer to the last event
  (this), as they are also well ordered in memory. An undersirable outcome of
  this is that sibling nodes of the per-process tree will *not* have equal
  depths, but depths that are now ordered, as well. This seems to have no impact
  if we are going to only use these clocks only for recovering causaly
  information inside of configurations.

