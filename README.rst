
==============================
DPU: Dynamic Program Unfolding
==============================

DPU is a research tool to perform `dynamic analysis`_ of POSIX multithreaded C
programs. It will automatically and exhaustively test all possible thread
schedules of a C program which uses `POSIX threads`_.

The tool instruments the source with a specific runtime that gets the control on
every call to a ``pthread_*`` function. That allows the tool (a) discover the
relevant interleavings and (b) to deterministically replay the program
execution.

DPU assumes that your program is **data-deterministic**, that is, the only
source of non-determinism in an execution is the order in which independent,
concurrent statements can be interleaved.  As a result, all sources of
non-deterministic execution (e.g., command-line arguments, input files) need to
be fixed before running the tool.

DPU also assumes that the program is **data-race free**, it will not detect data
races (I'm currently working on a new version that actually detects them, so
stay tunned), and it will not explore executions that "reverse" data races.

The tool implements optimal and non-optimal unfolding-based Partial-Order
Reduction (POR) algorithms extending those presented in our CONCUR'15 paper
(`arXiv:1507.00980`_).

.. _arXiv:1507.00980 : https://arxiv.org/abs/1507.00980
.. _dynamic analysis : https://en.wikipedia.org/wiki/Dynamic_program_analysis
.. _POSIX threads: https://en.wikipedia.org/wiki/POSIX_Threads

Dependencies
============

- coreutils
- git
- GNU make
- Python 2
- Clang 3.7
- LLVM 3.7
- `Steroids v0.1.0 <https://github.com/cesaro/steroids/releases/tag/v0.1.0>`__, a
  dynamic analysis library

Optional:

- Analyzing C programs with multiple compilation units will require
  `Whole Program LLVM <https://github.com/travitch/whole-program-llvm>`__.

Compilation
===========

Before compiling DPU please notice that:

- Development for DPU happens in the ``master`` branch. If you want a stable
  version of the tool you instead download and compile the
  `latest available release <https://github.com/cesaro/dpu/releases>`__ of the
  tool.
- DPU has only been compiled and tested under Debian/Ubuntu, although it should
  probably work on other Linux distributions. Please note that the Steroids
  library only works on x86-64 machines.

The steps here assume that you have a Debian/Ubuntu distribution:

1. Install a suitable development environment::

    sudo apt-get install coreutils git make python2.7

2. Install clang v3.7 and LLVM v3.7. DPU currently does not compile under g++,
   and you will need clang 3.7 to run the tool, anyway::
   
    sudo apt-get install llvm-3.7-dev clang-3.7

   After the installation, the command ``llvm-config-3.7`` should be in your
   ``PATH``, and typing ``llvm-config-3.7 --prefix`` should print the
   installation path of LLVM 3.7.

3. Download and compile `v0.1.0 <https://github.com/cesaro/steroids/releases/tag/v0.1.0>`__
   of the `Steroids dynamic analysis <https://github.com/cesaro/steroids>`__
   library. Using a different version of steroids may break the compilation or
   performance of DPU.

4. Download and compile the `latest release available
   <https://github.com/cesaro/dpu/releases>`__ for the DPU tool.

5. Edit the file `<config.mk>`__. Update the value of the variable
   ``CONFIG_STEROIDS_ROOT`` to point to the root of the steroids project.
   Give an absolute path or a path relative to the variable ``$R``,
   which will equal to the path of the root folder of the DPU project.

6. Compile::

    make dist

7. Optional: run regression tests::

    make regression

DPU is now installed in the ``dist/`` folder. You can run the tool from there
using the command::

 ./dist/bin/dpu --help

Installation
============

You can also install DPU elsewhere on your system. For that, move
the ``dist/`` directory to any location of your convenience, but make sure you do not
alter the internal contents of the folder. Include the directory ``dist/bin`` in your
``PATH`` and you are good to go.

Alternatively, you may update the value of the variable ``CONFIG_PREFIX`` in the
`<config.mk>`__ file. This way, ``make`` will copy the ``dist`` folder to the
installation directory every time you type ``make install``.

Tutorial
========

TODO

- Hello world
- Options
- wllvm

Related tools
=============

- `POET <https://github.com/marcelosousa/poet/>`__
- `Nidhugg <https://github.com/nidhugg/nidhugg>`__
- `CHESS <http://research.microsoft.com/chess/>`__
- `Maple <https://github.com/jieyu/maple>`__


Contact
=======

DPU is currently maintained by 
`César Rodríguez <http://lipn.univ-paris13.fr/~rodriguez/>`__.
Please feel free to contact me in case of questions or to send feedback.

