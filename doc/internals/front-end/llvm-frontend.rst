
LLVM Frontend + 3A Code transformations
=======================================

Passes:

1. Sanity Checks
   - all allocas in the first basic block, and has no predecessors
   - pthread_{create,join,mutex_init,mutex_lock,mutex_unlock}
   - pthread_create and pthread_join are not in a loop (or if)
   - all allocated variables are integers, or arrays of integers

2. Construction of 3A code
3. Large-Block Encoding

Pass 1: Sanity Checks
---------------------

Pass 2: Construction of 3A code
-------------------------------

Pass 3: Large-Block Encoding
----------------------------
