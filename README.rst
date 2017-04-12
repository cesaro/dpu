
==================================
DPU: Distributed Program Unfolding
==================================

DPU is a research tool to perform dynamic analysis of POSIX multithreaded C
programs.  It implements optimal and unoptimal unfolding-based Partial-Order
Reduction (POR) algorithms based on those presented in
`arXiv:1507.00980 <https://arxiv.org/abs/1507.00980>`__.

Dependencies
============

- coreutils
- git
- GNU make
- python2.7
- clang-3.7
- llvm-3.7-dev
- The `steroids dynamic analysis <https://github.com/cesaro/steroids>`__
  library.

Compilation
===========

DPU has ony been compiled and tested under Debian/Ubuntu, although it should
probably work on other Linux distributions. Please note that the Steroids
library only works on x86-64 machines.

The steps here assume that you have a Debian/Ubuntu distribution:

1. Install a suitable development environment::

    sudo apt-get install coreutils git make python2.7

2. Install clang-3.7 and LLVM v3.7. DPU currently does not compile under g++,
   and you will need clang-3.7 to run the tool, anyway::
   
    sudo apt-get install llvm-3.7-dev clang-3.7

   After the installation, the ``llvm-config-3.7`` should be in your ``PATH``,
   and the command ``llvm-config-3.7 --prefix`` should print the installation
   path to LLVM 3.7.

3. Install the `steroids dynamic analysis <https://github.com/cesaro/steroids>`__
   library.

4. Edit the file `<config.mk>`__. Update the value of the variable
   ``CONFIG_STEROIDS_ROOT`` to point to the path to the root of the steroids
   project. Give an absolute path or a path relative to the variable ``$R``,
   which will be equal to the path of the root of the DPU project.

5. Compile::

    make dist

6. Optional: run regression tests::

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

Related tools
=============

- `POET <https://github.com/marcelosousa/poet/>`__
- `Nidhugg <https://github.com/nidhugg/nidhugg>`__

Contact
=======

DPU is currently maintained by 
`César Rodríguez <http://lipn.univ-paris13.fr/~rodriguez/>`__.
Please feel free to contact me in case of questions or to send feedback.

