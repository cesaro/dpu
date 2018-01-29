
Compiling DPU
=============

Compiling DPU requires ``clang++`` v3.7, among other packages. Please also
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
- Clang 3.7
- LLVM 3.7
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

2. Install clang v3.7 and LLVM v3.7. DPU currently does not compile under g++,
   and you will need clang 3.7 to run the tool, anyway::

    sudo apt-get install llvm-3.7-dev clang-3.7

   After the installation, the command ``llvm-config-3.7`` should be in your
   ``PATH``, and typing ``llvm-config-3.7 --prefix`` should print the
   installation path of LLVM 3.7.

3. Download and compile `v0.2.0 <https://github.com/cesaro/steroids/releases/tag/v0.2.0>`__
   of the `Steroids dynamic analysis <https://github.com/cesaro/steroids>`__
   library. Using a different version of steroids may break the compilation or
   performance of DPU.

4. Download and compile the sources of the `latest release`_ available.

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

