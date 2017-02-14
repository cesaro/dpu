
Status
======

- pi, we take it
- c-ring could work with a modification so that all thread exit
- mutex-unlock/2-1 -> ok but very simple, and there is bug?
- philo: semaphores -> mutexes

- astrometry.net
- ogg123

- ssbexp, ssb3 -> Marcelo

Not used:

- runtime/uncontested -> trivial
- runtime/contested -> uses a signal to stop an infinite loop :(


Commandline arguments
=====================

dpu pi/pth_pi_mutex.c -- pi 4 8192
