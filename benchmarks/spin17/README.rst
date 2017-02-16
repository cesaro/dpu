
Status
======

- pi, we take it
- mutex-unlock/2-1 -> ok but very simple, and there is bug?
- philo: semaphores -> mutexes

- astrometry.net
- ogg123

- ssbexp, ssb3 -> turn into realistic programs, Marcelo

Not used:

- c-ring, illegal use of posix mutexes (threads unlock mutexes not belonging to them)
- runtime/uncontested -> trivial
- runtime/contested -> uses a signal to stop an infinite loop :(


Commandline arguments
=====================

dpu pi/pth_pi_mutex.c -- pi 4 8192
