
3-Address Internal Representation
=================================

Examples
--------

::
 move i32    [0x10] [0x14]        ; copy 4 bytes from address 0x10 to 0x14
 move i8     [0x10] [0x14]        ; copy 1 byte from address 0x10 to 0x14
 move i8     [0x10] 0x56          ; write to address 0x10 the mmediate byte value 0x56
 move float  [0x10] 3.141516      ; write 4 bytes in IEEE single precision format to address 0x10
 move double [0x10] 3.1415        ; write 8 bytes in IEEE double precissin

 imov i8     [0x10] [[0x14]]      ; reads 4 bytes at address 0x14, interprets them
                                  ; as an address, and transfers 1 byte from that
                                  ; address to address 0x10
 
 add  i32    [0x10] [0x14] 8      ; adds immediate 8 to the 32bits integer
                                  ; stored at address 0x14, storing the result
                                  ; at address 0x10
 add  i32    [0x10] [0x14] [0x20] ; adds variables 0x14 and 0x20, stores at
                                  ; address 0x10
 add  float  [0x10] [0x14] [0x20]
 add  double [0x10] [0x14] [0x20]

 cmp eq i8   [0x10] [0x14] 6      ; tests that byte at address 0x11 equals 6
                                  ; stores an i32 with value 1 at address 0x10
                                  ; if so; else it stores 0
 cmp ult i16 [0x10] [0x14] [0x16] ; "unsigned less than" test
 br          123                  ; jump to program counter 123
 br          [0x10] 22 33         ; jump to PC 22 if the i32 stored at address
                                  ; 0x10 is non-zero; else jump to PC 33


 ...

Adressing Modes
---------------

- Immediate operands, given directly on the instruction
- Absolute/Direct access: address of the data to access is given in the
  instruction operand
- Indirect access: only the instruction ``imov`` is able to perform indirect
  access to data

Types
-----

Operand sizes:

- i8        = 1
- i16       = 2
- i32       = 4
- i64       = 8
- float     = 4
- double    = 8
- arbitrary = between 0 and 2^32-1

Integer type = 1, 2, 4, or 8
Float type   = 4 or 8

Operation Codes, Syntax and "Semantics"
---------------------------------------

T   = Type
DST = Destination address (address)
SRC = Source address or immediate value (address, integer, or float, at most 32bits)
DI  = Indirect addres, (the address of an address)
LAB = Label

=========================== ====================================================
Syntax                      Description
=========================== ====================================================
nop                         No operation
error                       This instruction should never be reached
ret     T SRC               Terminates the function
=========================== ====================================================
move    T DST SRC           Moving data or storing immediate values
imov    T DST DI            Indirect move from address pointed by DI to DST
=========================== ====================================================
cmp eq  T DST SRC1 SRC2     Comparison: equal
cmp ne  T DST SRC1 SRC2     Comparison: not equal
cmp ugt T DST SRC1 SRC2     Comparison: unsigned greater than
cmp uge T DST SRC1 SRC2     Comparison: unsigned greater or equal
cmp ult T DST SRC1 SRC2     Comparison: unsigned less than
cmp ule T DST SRC1 SRC2     Comparison: unsigned less or equal
cmp sgt T DST SRC1 SRC2     Comparison: signed greater than
cmp sge T DST SRC1 SRC2     Comparison: signed greater or equal
cmp slt T DST SRC1 SRC2     Comparison: signed less than
cmp sle T DST SRC1 SRC2     Comparison: signed less or equal
brz       SRC LAB           Jump if zero (SRC is i32)
brnz      SRC LAB           Jump if non zero (SRC is i32)
=========================== ====================================================
add     T DST SRC1 SRC2     Addition
sub     T DST SRC1 SRC2     Substraction
mul     T DST SRC1 SRC2     Multiplication
sdiv    T DST SRC1 SRC2     Signed division; expects integer type
udiv    T DST SRC1 SRC2     Unsigned division; expects integer type
srem    T DST SRC1 SRC2     Remainder of signed division; expects integer type
urem    T DST SRC1 SRC2     Remainder of unsigned division; expects integer type
fdiv    T DST SRC1 SRC2     ## Division; expects float type
frem    T DST SRC1 SRC2     ## Remainder of division; expects float type
=========================== ====================================================
or      T DST SRC1 SRC2     Bitwise logical or; expects integers
and     T DST SRC1 SRC2     Bitwise logical and; expects integers
xor     T DST SRC1 SRC2     Bitwise logical xor; expects integers
=========================== ====================================================
sext    T1 T2 DST SRC       Signed extension from type T1 to T2; expects integer
zext    T1 T2 DST SRC       Zero extends SRC from type T1 to T2; expects integer
=========================== ====================================================
lock    T SRC               Locks a mutex; expects address of integer operand
unlock  T SRC               Unlocks a mutex; expects address of integer operand
printf  T FMT SRC1 SRC2     Printf, where FMT admits 0, 1 or 2 "%d"
=========================== ====================================================
br        LAB               Unconditional branch (nop)
br        SRC LAB1 LAB2     Conditional branch
=========================== ====================================================


Example
-------

Symbol table:

===== ===== ======
addr  type  symbol
===== ===== ======
0x04  i32   x
0x08  i32   y
0x0c  i32   i
0x10  i32   acc
0x14  i32   cnd
===== ===== ======

Program::
 entry:
   move i32 [x] 2
   move i32 [y] 5
   move i32 [i] 0
   move i32 [acc] 0
   br loopend
 
 loophead:
   add i32 [acc] [acc] [x]
   add i32 [i] [i] 1
 
 loopend:
   cmp ult i32 [cnd] [i] [y]
   br [cnd] loophead loopexit
 
 loopexit:
   mul i32 [i] [x] [y]
   cmp ne i32 [cnd] [acc] [i]
   br [cnd] fault term
 
 fault:
   error
 
 term:
   ret i32 0

Equivalent C program::
 void test ()
 {
   int x = 2;
   int y = 5;
   int i;
   int acc = 0;

   for (i = 0; i < y; i++) acc += y

   assert (acc == x * y);
 }

Tentative Translation from LLVM
-------------------------------

TBD

%add = alloca i32         | MOVE i32 [[add]] [3]
%val = load i32* add      | IMOVE i32 [val] [add]
store i32 %4, i32* %i     | MOVE i32 [[i]] [%4]
ret i32 %10               | RET i32 %10
ret i8* null              | RET
br i1 %3, label %4, label %8 | BR %3 0xaf8610 0xaf8670
br label %11              | BR 0x1c80d90
%3 = icmp sgt i32 3, %2   | CMP sgt %3 3 %2




Data Structures
---------------

========= ===== ===== ===== ======== ============================= =================================
opcode    type  dst   src1  src2     text                          comments
========= ===== ===== ===== ======== ============================= =================================
NOP       -     -     -     -        nop
ERROR     -     -     -     -        error
RET       T     -     SRC   -        ret T [SRC]                   
RETI4     T     -     IMM   -        ret T IMM                     T is 1-4
RETI8     T     -     IMM1  IMM2     ret T IMM                     T is 5-8, IMM1 low, IMM2 high
MOVE      T     DST   SRC   -        move T [DST] [SRC]
MOVEI4    T     DST   IMM   -        move T [DST] IMM              T is 1-4
MOVEI8    T     DST   IMM1  IMM2     move T [DST] IMM              T is 5-8, IMM1 low, IMM2 high
IMOV      T     DST   SRC   -        imov T [DST] [[SRC]]
CMP_EQ    T     DST   SRC1  SRC2     cmp eq T [DST] [SRC1] [SRC2]
CMP_EQI4  T     DST   SRC1  IMM      cmp eq T [DST] [SRC] IMM      T is 1-4
========= ===== ===== ===== ======== ============================= =================================
-> similarly for cmp ule, uge, ult, sgt, sge, slt, sle
BRZ       -     -     SRC   -        brz [SRC] LAB                 SRC interpreted as 32 bits
BRNZ      -     -     SRC   -        brz [SRC] LAB                 SRC interpreted as 32 bits
========= ===== ===== ===== ======== ============================= =================================
ADD       T     DST   SRC1  SRC2     add T [DST] [SRC1] [SRC2]
ADDI4     T     DST   SRC1  IMM      add T [DST] [SRC1] IMM        T is 1-4
ADDI8     T     DST   IMM1  IMM2     add T [DST] [DST] IMM         T is 5-8, IMM1 low, IMM2 high
-> similarly for sub, mul
-> sdiv, udiv, srem, urem only accepts T from 1 to 4, so opcodes SDIV and SDIVI4, and so on
========= ===== ===== ===== ======== ============================= =================================
OR        T     DST   SRC1  SRC2     or T [DST] [SRC1] [SRC2]
ORI4      T     DST   SRC   IMM      or T [DST] [SRC] IMM          T is 1-4
-> similarly for and, xor
========= ===== ===== ===== ======== ============================= =================================
SEXT      T1    DST   SRC   T2       sext T1 T2 [DST] [SRC]        T1 < T2 <= 8
-> similarly for zext
========= ===== ===== ===== ======== ============================= =================================
LOCK      T     DST   -     -        lock T [DST]   
UNLOCK    T     DST   -     -        lock T [DST]   
PRINTF    T     FMT   SRC1  SRC2     printf T [FMT] [SRC1] [SRC2]
========= ===== ===== ===== ======== ============================= =================================
