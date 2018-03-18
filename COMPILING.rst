
Compiling DPU
=============

Compiling DPU requires ``clang++-6.0``, among other packages. Please also
notice that:

- The ``master`` branch in this project always points to the version of the code
  in the `latest release`_ of the tool.  The development version is available in
  the ``develop`` branch.
- DPU has only been compiled and tested under Debian/Ubuntu, although it should
  probably work on other Linux distributions. Please note that the Steroids
  library only works on x86-64 machines.

.. _latest release : https://github.com/cesaro/dpu/releases/latest

Dependencies
------------

- coreutils
- git
- GNU make
- Python 2
- Clang 6.0
- LLVM 6.0
- The Steroids dynamic analysis library (included as a `Git submodule`_)

Optional:

- Analyzing C programs with multiple compilation units will require
  `Whole Program LLVM`_.

.. _Git submodule: https://git-scm.com/book/en/v2/Git-Tools-Submodules
.. _Whole Program LLVM: https://github.com/travitch/whole-program-llvm

Compilation
-----------

The following steps assume that you have a Debian/Ubuntu distribution:

1. Install a suitable development environment::

    sudo apt-get install coreutils git make python2.7

2. Install Clang v6.0 and LLVM v6.0 (see the `LLVM releases`_). DPU currently does not
   compile under ``g++``, and you will need Clang 6.0 to run the tool anyway::

    sudo apt-get install llvm-6.0-dev clang-6.0

   After the installation, the command ``llvm-config-6.0`` should be in your
   ``PATH``, and typing ``llvm-config-6.0 --prefix`` should print the
   installation path of LLVM 6.0.

3. Clone this project and initialize the git submodule containing the Steroids
   library::

    git clone https://github.com/cesaro/dpu.git
    cd dpu
    git submodule init
    git submodule update

4. A number of compilation-time configuration parameters are available in the
   file `<config.mk>`__. All of them have a safe default values but you might
   want to modify any of them now.

5. Compile the tool::

    make dist

6. Optional: run regression tests::

    make regression

DPU is now installed in the ``dist/`` folder. You can run the tool from there
using the command::

 ./dist/bin/dpu --help

.. _LLVM releases : http://releases.llvm.org/download.html#6.0.0

Installing the dist/ folder
===========================

You can also install DPU elsewhere on your system. For that, move
the ``dist/`` directory to any location of your convenience, but make sure you do not
alter the internal contents of the folder. Include the directory ``dist/bin`` in your
``PATH`` and you are good to go.

Alternatively, you may update the value of the variable ``CONFIG_PREFIX`` in the
`<config.mk>`__ file. This way, ``make`` will copy the ``dist`` folder to the
installation directory every time you type ``make install``.
