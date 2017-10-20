
Experiments with the SVCOMP'17 Benchmarks
=========================================

======================== ======================================================
Folder                   Status
======================== ======================================================
``pthread``              Almost all benchmarks in this folder run with DPU.
``pthread-C-DAC``        TBD
``pthread-atomic``       TBD
``pthread-complex``      TBD
``pthread-driver-races`` TBD      
``pthread-ext``          TBD
``pthread-lit``          TBD
``pthread-wmm``          DPU does not suppot weak memory, no benchmark in this
                         folder can be anlayzed with DPU.
======================== ======================================================

How the benchmarks were obtained
================================

On November 2, 2017, we went to
https://github.com/sosy-lab/sv-benchmarks/tree/master/c and listed all the
relevant directories::

 pthread
 pthread-C-DAC
 pthread-atomic
 pthread-complex
 pthread-driver-races
 pthread-ext
 pthread-lit
 pthread-wmm

We now produced a list of the files in each of the folders:

.. code:: shell

 for i in pthread pthread-C-DAC pthread-atomic pthread-complex \
    pthread-driver-races pthread-ext pthread-lit
 do
    svn ls https://github.com/sosy-lab/sv-benchmarks.git/trunk/c/$i > $i;
 done

Files downloaded by running the command:

.. code:: shell

 wget -x -i URLs

The files in folder ``c/pthread-driver-races/model`` were downloaded by other
means.

