
ssbexp con K = 5

== start ==
m: reading 0
m: locking 0
w0: locking
w1: locking
w2: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
== end ==

== start ==
m: reading 0
w0: locking
m: locking 0
w1: locking
w2: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
== end ==
== start ==
w0: locking
w1: locking
w2: locking
w3: locking
w4: locking
count: i 1
m: reading 1
m: locking 1
count: i 2
count: i 3
count: i 4
== end ==
== start ==
w0: locking
w1: locking
w2: locking
w3: locking
w4: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
count: i 3
count: i 4
== end ==
== start ==
w0: locking
w1: locking
w2: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
count: i 4
== end ==
== start ==
w0: locking
w1: locking
w2: locking
w3: locking
w4: locking    --
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4   **
== end ==

Aqui tiene w0 a w4 en la pila, y para cada uno de ellos ha encontrado una
alternativa, ha explorado "m: reading i" para i = 1 a 4.

== start ==
w0: locking
w1: locking
w2: locking
w3: locking
count: i 1     w4 duerme aqui; ahora va a explorar i=1 a i=4, y solo en la
               ultima lo logara justificar
m: reading 1
m: locking 1
count: i 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w2: locking
w3: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w2: locking
w3: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
count: i 4

== start ==
w0: locking
w1: locking
w2: locking
w3: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4      Aqui lo ha logrado justificar
w4: locking
== end ==

== start ==
w0: locking
w1: locking
w2: locking
w4: locking       Ahora pone w3 a dormir aqui, y de nuevo explora i=1 a i=4,
                  solo con i=3 lo logra justificar
                  La CLAVE es que en lugar de ir a count i xx, ha ejecutado w4
                  IMMEDIATAMENTE despues de poner a dormir w3, en lugar de
                  ejecutar la alternativa, que comenzaba con i 1; en realidad w4
                  podia hacer parte de la alternativa, si era una de sus wide
                  enabled alternatives or whatever they call it. Esto hace que
                  los errores de decision "se acumulen" en la pila, construyendo
                  la explosion combinatoria exponencial. En efecto esta tomando
                  la alternativa a partir de WI_[E'] en lugar de W_[E'].
count: i 1
m: reading 1
m: locking 1
count: i 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w2: locking
w4: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w2: locking
w4: locking
count: i 1
count: i 2
count: i 3
m: reading 3      Justificado aqui, al leer idx = i = 3.
m: locking 3
w3: locking
count: i 4
== end ==

== start ==
w0: locking
w1: locking
w2: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4      Mismo problema cuando m lee 4.
m: locking 4


== start ==
w0: locking
w1: locking
w2: locking
count: i 1        
m: reading 1
m: locking 1
count: i 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w2: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w2: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
w3: locking
count: i 4

== start ==
w0: locking
w1: locking
w2: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
w4: locking

== start ==
w0: locking
w1: locking
w3: locking    Pone w2 a dormir aqui, y ejecuta immediatamente despues w3 y w4,
               lo cual le lace entrar en el mismo problema que antes
w4: locking
count: i 1
m: reading 1
m: locking 1
count: i 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w3: locking
w4: locking
count: i 1
count: i 2
m: reading 2   
m: locking 2
w2: locking    Consigue justificar w2 aqui, y no lo conseguira en las ejecucions
               siguientes
count: i 3
count: i 4
== end ==

== start ==
w0: locking
w1: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
count: i 4

== start ==
w0: locking
w1: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4

== start ==
w0: locking
w1: locking
w3: locking    Ahora D = 2, w2 duerme aqui
count: i 1     y w4 duerme aqui, pero esta condenado a encontrar unicamente SSBs
m: reading 1
m: locking 1
count: i 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w3: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
w2: locking    Consigue justificar w2 un paso antes de aqui, pero no w4 en esta ejecucion
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w3: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
count: i 4

== start ==
w0: locking
w1: locking
w3: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4   Justifica w4 pero no w2
w4: locking

== start ==
w0: locking
w1: locking
w4: locking
count: i 1
m: reading 1
m: locking 1
count: i 2
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w4: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
w2: locking
count: i 3
count: i 4

== start ==
w0: locking
w1: locking
w4: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
w3: locking
count: i 4

== start ==
w0: locking
w1: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
== start ==
w0: locking
w1: locking
count: i 1
m: reading 1
m: locking 1
count: i 2
count: i 3
count: i 4
== start ==
w0: locking
w1: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
w2: locking
count: i 3
count: i 4
== start ==
w0: locking
w1: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
w3: locking
count: i 4
== start ==
w0: locking
w1: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
w4: locking
== start ==
w0: locking
w2: locking
w3: locking
w4: locking
count: i 1
m: reading 1
m: locking 1
w1: locking
count: i 2
count: i 3
count: i 4
== end ==
== start ==
w0: locking
w2: locking
w3: locking
w4: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
count: i 3
count: i 4
== start ==
w0: locking
w2: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
count: i 4
== start ==
w0: locking
w2: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
== start ==
w0: locking
w2: locking
w3: locking
count: i 1
m: reading 1
m: locking 1
w1: locking
count: i 2
count: i 3
count: i 4
== start ==
w0: locking
w2: locking
w3: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
count: i 3
count: i 4
== start ==
w0: locking
w2: locking
w3: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
count: i 4
== start ==
w0: locking
w2: locking
w3: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
w4: locking
== start ==
w0: locking
w2: locking
w4: locking
count: i 1
m: reading 1
m: locking 1
w1: locking
count: i 2
count: i 3
count: i 4
== start ==
w0: locking
w2: locking
w4: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
count: i 3
count: i 4
== start ==
w0: locking
w2: locking
w4: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
w3: locking
count: i 4
== start ==
w0: locking
w2: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
== start ==
w0: locking
w2: locking
count: i 1
m: reading 1
m: locking 1
w1: locking
count: i 2
count: i 3
count: i 4
== start ==
w0: locking
w2: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
count: i 3
count: i 4
== start ==
w0: locking
w2: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
w3: locking
count: i 4
== start ==
w0: locking
w2: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
w4: locking
== start ==
w0: locking
w3: locking
w4: locking
count: i 1
m: reading 1
m: locking 1
w1: locking
count: i 2
count: i 3
count: i 4
== start ==
w0: locking
w3: locking
w4: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
w2: locking
count: i 3
count: i 4
== start ==
w0: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
count: i 4
== start ==
w0: locking
w3: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
== start ==
w0: locking
w3: locking
count: i 1
m: reading 1
m: locking 1
w1: locking
count: i 2
count: i 3
count: i 4
== start ==
w0: locking
w3: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
w2: locking
count: i 3
count: i 4
== start ==
w0: locking
w3: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
count: i 4
== start ==
w0: locking
w3: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
w4: locking
== start ==
w0: locking
w4: locking
count: i 1
m: reading 1
m: locking 1
w1: locking
count: i 2
count: i 3
count: i 4
== start ==
w0: locking
w4: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
w2: locking
count: i 3
count: i 4
== start ==
w0: locking
w4: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
w3: locking
count: i 4
== start ==
w0: locking
w4: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
== start ==
w0: locking
count: i 1
m: reading 1
m: locking 1
w1: locking
count: i 2
count: i 3
count: i 4
== start ==
w0: locking
count: i 1
count: i 2
m: reading 2
m: locking 2
w2: locking
count: i 3
count: i 4
== start ==
w0: locking
count: i 1
count: i 2
count: i 3
m: reading 3
m: locking 3
wTrace count: 10 (also 56 sleepset blocked)
No errors were detected.
3: locking
count: i 4
== start ==
w0: locking
count: i 1
count: i 2
count: i 3
count: i 4
m: reading 4
m: locking 4
w4: locking
* Nidhuggc: $ /usr/bin/clang-3.4 -o /tmp/orig.ll -S -emit-llvm -g ssbexp.c
* Nidhuggc: $ /home/cesar/x/devel/nidhugg/src/nidhugg -transform /tmp/trans.ll /tmp/orig.ll
* Nidhuggc: $ /home/cesar/x/devel/nidhugg/src/nidhugg -extfun-no-race=printf -extfun-no-race=write -extfun-no-race=exit -extfun-no-race=atoi -sc /tmp/trans.ll
Total wall-clock time: 0.07 s
