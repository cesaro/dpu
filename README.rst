
==============================
DPU: Dynamic Program Unfolding
==============================

DPU is a research tool to perform `dynamic analysis`_ of POSIX multithreaded C
programs. It will automatically and exhaustively test all possible thread
schedules of a C program that uses `POSIX threads`_.

The tool instruments the source with a specific runtime that gets the control on
every call to a ``pthread_*`` function. That way it can (a) discover the
relevant thread interleavings and (b) control the thread scheduler to
deterministically replay threaded executions.

.. _dynamic analysis : https://en.wikipedia.org/wiki/Dynamic_program_analysis
.. _POSIX threads: https://en.wikipedia.org/wiki/POSIX_Threads

Detected Flaws
==============

For each execution of the program DPU can currently detect the following flaws:

- Assertion violations, as triggered by the function `assert(3)`_.
- Calls to `abort(3)`_.
- Executions where the program calls `exit(3)`_ with a non-zero exit code. This
  is not necessarily a flaw, but a non-zero exit code usually denotes that an
  error ocurred.
- Data races.
- Deadlocks.

.. _assert(3) : http://man7.org/linux/man-pages/man3/assert.3.html
.. _abort(3) : http://man7.org/linux/man-pages/man3/abort.3.html
.. _exit(3) : http://man7.org/linux/man-pages/man3/exit.3.html

Assumptions About the Input Program
===================================

DPU assumes that your program is **data-deterministic**, that is, the only
source of non-determinism in an execution is the order in which independent,
concurrent statements can be interleaved.  As a result, all sources of
non-deterministic execution (e.g., command-line arguments, input files) need to
be fixed before running the tool.

Data-race Detection
===================

DPU can detect and report data-races in the input program (option ``-a dr``),
but those will not be used as source of thread interference to find new thread
interleavings.  The tool will not explore two execution orders for the two
instructions that exhibit a data-race.

When data-race analysis is enabled, DPU will record memory load/store operations
performed by the program, in addition to the calls to ``pthread_*`` functions.
This detection happens for a user-provided percentage (option ``--drfreq``,
default 10%) of the executions explored by the tool. This analysis is thus not
guaranteed to find all data-races, but any data-race found is a genuine one.

Exploration Algorithm
=====================

The tool explores the state-space of thread interleavings using optimal and
non-optimal unfolding-based Partial-Order Reduction (POR) algorithms extending
those presented in our CONCUR'15 paper (`arXiv:1507.00980`_).

.. _arXiv:1507.00980 : https://arxiv.org/abs/1507.00980


Installing Precompiled Binaries
===============================

The following steps assume that you have a Debian/Ubuntu distribution:

1. Install Clang v6.0 and LLVM v6.0 (see the `LLVM releases`_)::

    sudo apt-get install clang-6.0 llvm-6.0

   The commands ``clang-6.0`` and ``llvm-link-6.0`` should now be available in
   your ``$PATH``.

2. Download the precompiled binaries from the `latest release`_ and unpack them
   anywhere in your machine.

3. The DPU tool is located within the folder ``dpu-vx.x.x/bin`` in the
   downloaded package. You can either run it from there or update your
   ``$PATH`` variable to include this folder. In the second case add the
   following line to your ``~/.bashrc`` file *and* restart your teminal::

    export PATH=$PATH:/path/to/dpu-vx.x.x/bin

4. You should now be able to run::

    dpu --help
    dpu --version

.. _LLVM releases : http://releases.llvm.org/download.html#6.0.0
.. _latest release : https://github.com/cesaro/dpu/releases/latest

Compilation
===========

Instructions for compiling from the sources are available in the
`<COMPILING.rst>`__ file.

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
