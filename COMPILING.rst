
Compiling DPU
=============

Compiling DPU requires ``clang++`` v6.0, among other packages. Please also
notice that:

- Development for DPU happens in the ``master`` branch. If you want a stable
  version of the tool you should download and compile the sources of the
  `latest available release <https://github.com/cesaro/dpu/releases>`__ of the
  tool.
- DPU has only been compiled and tested under Debian/Ubuntu, although it should
  probably work on other Linux distributions. Please note that the Steroids
  library only works on x86-64 machines.

Dependencies
------------

- coreutils
- git
- GNU make
- Python 2
- Clang 6.0
- LLVM 6.0
- `Steroids v0.2.0 <https://github.com/cesaro/steroids/releases/tag/v0.2.0>`__, a
  dynamic analysis library

Optional:

- Analyzing C programs with multiple compilation units will require
  `Whole Program LLVM <https://github.com/travitch/whole-program-llvm>`__.

Compilation
-----------

The following steps assume that you have a Debian/Ubuntu distribution:

1. Install a suitable development environment::

    sudo apt-get install coreutils git make python2.7

2. Install Clang v6.0 and LLVM v6.0 (binaries and packages available `here
   <http://releases.llvm.org/download.html#6.0.0>`__). DPU currently does not
   compile under ``g++``, and you will need Clang 6.0 to run the tool, anyway::

    sudo apt-get install llvm-6.0-dev clang-6.0

   After the installation, the command ``llvm-config-6.0`` should be in your
   ``PATH``, and typing ``llvm-config-6.0 --prefix`` should print the
   installation path of LLVM 6.0.

3. Download and compile `v0.2.0 <https://github.com/cesaro/steroids/releases/tag/v0.2.0>`__
   of the `Steroids dynamic analysis <https://github.com/cesaro/steroids>`__
   library. Using a different version of steroids may break the compilation or
   performance of DPU.

4. Download and compile the `latest release`_ available.

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

Installing the dist/ folder
===========================

You can also install DPU elsewhere on your system. For that, move
the ``dist/`` directory to any location of your convenience, but make sure you do not
alter the internal contents of the folder. Include the directory ``dist/bin`` in your
``PATH`` and you are good to go.

Alternatively, you may update the value of the variable ``CONFIG_PREFIX`` in the
`<config.mk>`__ file. This way, ``make`` will copy the ``dist`` folder to the
installation directory every time you type ``make install``.

.. _latest release : https://github.com/cesaro/dpu/releases/latest

