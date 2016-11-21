
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
- how to give semantics to failing create/join
- how to represent conflict?
- steroids -> unf
- design unfolder class

Huyen:
- vclocks
- BaseConfig -> replay
- Unfolding::print_dot

Improvements
============

- Better algorithm for finding pre-existing events in the unfolding when we
  instert new events, based on a radix tree or hash table per event; or at least
  optimize to scan the smallest postset in Unfolding::find2


