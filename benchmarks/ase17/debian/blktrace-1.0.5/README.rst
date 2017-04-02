
The files and folders in this folder come from the Ubuntu package blkiotrace
v.1.0.5, available here:

http://packages.ubuntu.com/trusty/blktrace

The only program in the package that uses POSIX threads is ``blkiomon``. We have
preseved here the sources of only this program.

You would normally run it like this::

 blktrace /dev/sdw -a issue -a complete -w 3600 -o - | blkiomon -I 10 -h -

We added 3 new commandline arguments:

- -i PATH, where PATH is a file to read disk tracing records, as they would
  otherwise be read from the standard input
- -x N, where N will be the maximum number of iterations of the main loop
- -y M, where M will be the maximum number of iterations of the thread loop

To run the modified copy we first capture a file with data records::

 sudo blktrace -d /dev/nvme0n1p3 -o - | tee input.dat | hexdump -C

We then run our modified copy like this::

 blkiomon -i input.dat -I 1 -y 5 -x 23 -h -

DPU explores 11K configurations in 5 seconds::

 dpu blkiomon.full.bc -O3 -- blkiomon -i input1.dat -I 1 -y 5 -x 23 -h -

