
Instructions for CAV'18 Reviewers
=================================

This file contains:

- statically linked binaries of the DPU tool
- the source code of the DPU tool
- scripts to run the tool

Please refer to the README.rst file for further information about DPU.

Quickstart
==========

The tool binary is located at ``bin/dpu``. You can run the tool from that folder
but you should not move the ``dpu`` binary to another folder (as the binary
finds the ``lib/`` folder using a relative file path).

Run the tool with ``--version`` and ``-h`` to get information about its
commandline options::

 $ dpu --version
 dpu v0.5.0 (d73ecfa), compiled Wed, 31 Jan 2018 22:36:32 +0100
 Build type: release
 [...]

 $ ./bin/dpu  -h
 Usage: dpu FILE.{c,i,bc,ll} ANALYZEROPTS -- PROGRAMOPTS
 [...]

Run Optimal DPOR on the benchmark ``multiprodcon.c``::

 ./bin/dpu experiments/cav18/bench/multiprodcon.c -k0

Same as above but use QPOR with 2-partial alternatives::

 ./bin/dpu experiments/cav18/bench/multiprodcon.c -k2

Same as above but also check for data races on half of the program schedulings analized::

 ./bin/dpu experiments/cav18/bench/multiprodcon.c -k2 -a dr --drfreq 50


Running the Experiments in the Paper
====================================

All necessary information to reproduce the results of Tables 1, 2, and 3 can be
found within the folder ``experiments/cav18``. Please refer to the README inside
that folder.
