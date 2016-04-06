
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

T    = Type
DST  = Destination address (address)
IDST = Indirect Destination address (the address of an address)
SRC  = Source address or immediate value (address, integer, or float, at most 32bits)
DI   = Indirect addres, (the address of an address)
LAB  = Label

=========================== ====================================================
Syntax                      Description
=========================== ====================================================
error                       This instruction should never be reached
ret     T SRC               Terminates the function
=========================== ====================================================
move    T DST SRC           Moving data or storing immediate values
movis   T DST DI            Move from indirect source address to destination
movid   T IDST SRC          Move to indirect destination address from source
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
br        SRC LAB1 LAB2     Branch to LAB1 if SRC is non-zero; otherwise LAB2
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
lock    T SRC               Locks a mutex; expects address of 32 bit operand
unlock  T SRC               Unlocks a mutex; expects address of integer operand
printf  T FMT SRC1 SRC2     Printf, where FMT admits 0, 1 or 2 "%d"
=========================== ====================================================
br        LAB               Unconditional branch (nop)
brz       SRC LAB           Jump if zero (SRC is i32)
brnz      SRC LAB           Jump if non zero (SRC is i32)
=========================== ====================================================


Example
-------

Symbol table:

========== ===== ======= =======
Address    Size  Initial Symbol
========== ===== ======= =======
0x00       i32   0       __pc0
0x04       i32   2       x
0x08       i32   5       y
0x0c       i32   0       i
0x10       i32   0       acc
0x14       i32   0       cnd
========== ===== ======= =======

Program::

 Function "main"
 ===============
 entry:
   move i32 [x] 2
   move i32 [y] 5
   move i32 [i] 0
   move i32 [acc] 0
 
 loopend:
   cmp slt i32 [cnd] [i] [y]
   br [cnd] loophead loopexit
 
 loophead:
   add i32 [acc] [acc] [x]
   add i32 [i] [i] 1
   br loopend
 
 loopexit:
   mul i32 [i] [x] [y]
   cmp ne i32 [cnd] [acc] [i]
   br [cnd] fault term
 
 fault:
   error
 
 term:
   ret i32 0

 End
 ===

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

%7 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([7 x i8]* @.str, i32 0, i32 0)) | PRINTF i32 %7 "i8* getelementptr inbounds ([7 x i8]* @.str, i32 0, i32 0)"
%1 = call i32 @pthread_mutex_lock(%union.pthread_mutex_t* @mut) | LOCK i32 [3] ; ignore the returned value





Data Structures
---------------

========= ===== ===== ===== ======== ============================= =================================
opcode    size  dst   src1  src2     text                          comments
========= ===== ===== ===== ======== ============================= =================================
ERROR     1     -     -     -        error
RET       T     -     SRC   -        ret T [SRC]                   
RETI      T     -     -     IMM      ret T IMM                     
MOVE      T     DST   SRC   -        move T [DST] [SRC]
MOVEI     T     DST   -     IMM      move T [DST] IMM
MOVIS     T     DST   SRC   -        movi T [DST] [[SRC]]
MOVID     T     DST   SRC   -        movi T [[DST]] [SRC]
==MOVIDI    T     DST   -     IMM      movi T [[DST]] IMM
========= ===== ===== ===== ======== ============================= =================================
CMP_EQ    T     DST   SRC1  SRC2     cmp eq T [DST] [SRC1] [SRC2]  SRCx size T; DST is i32
CMP_EQI   T     DST   SRC   IMM      cmp eq T [DST] [SRC] IMM      
-> similarly for cmp ule, uge, ult, sgt, sge, slt, sle
========= ===== ===== ===== ======== ============================= =================================
BR        4     -     SRC   LAB2     br i32 [SRC] LAB1 LAB2        SRC is i32; LAB1 external, LAB2 i64
BRZ       4     -     SRC   -        brz i32 [SRC] LAB             SRC interpreted as 32 bits
BRNZ      4     -     SRC   -        brz i32 [SRC] LAB             SRC interpreted as 32 bits
========= ===== ===== ===== ======== ============================= =================================
ADD       T     DST   SRC1  SRC2     add T [DST] [SRC1] [SRC2]
ADDI      T     DST   SRC   IMM      add T [DST] [SRC] IMM        
-> similarly for mul, or, and, xor
SUB       T     DST   SRC1  SRC2     sub T [DST] [SRC1] [SRC2]     DST = SRC1 - SRC2
SUBI      T     DST   SRC   IMM      sub T [DST] IMM [SRC]         DST = IMM - SRC
SDIV      T     DST   SRC1  SRC2     sdiv T [DST] [SRC1] [SRC2]    DST = SRC1 / SRC2 (signed)
SDIVIA    T     DST   SRC   IMM      sdiv T [DST] IMM [SRC]        DST = IMM / SRC   (signed)
SDIVAI    T     DST   SRC   IMM      sdiv T [DST] [SRC] IMM        DST = SRC / IMM   (signed)
UDIV      T     DST   SRC1  SRC2     udiv T [DST] [SRC1] [SRC2]    DST = SRC1 / SRC2 (unsigned)
UDIVIA    T     DST   SRC   IMM      udiv T [DST] IMM [SRC]        DST = IMM / SRC   (unsigned)
UDIVAI    T     DST   SRC   IMM      udiv T [DST] [SRC] IMM        DST = SRC / IMM   (unsigned)
SREM      T     DST   SRC1  SRC2     srem T [DST] [SRC1] [SRC2]    DST = SRC1 % SRC2 (signed)
SREMIA    T     DST   SRC   IMM      srem T [DST] IMM [SRC]        DST = IMM % SRC   (signed)
SREMAI    T     DST   SRC   IMM      srem T [DST] [SRC] IMM        DST = SRC % IMM   (signed)
UREM      T     DST   SRC1  SRC2     urem T [DST] [SRC1] [SRC2]    DST = SRC1 % SRC2 (unsigned)
UREMIA    T     DST   SRC   IMM      urem T [DST] IMM [SRC]        DST = IMM % SRC   (unsigned)
UREMAI    T     DST   SRC   IMM      urem T [DST] [SRC] IMM        DST = SRC % IMM   (unsigned)
========= ===== ===== ===== ======== ============================= =================================
SEXT      T1    DST   SRC   T2       sext T1 to T2 [DST] [SRC]     1 <= T1 < T2 <= 8
-> similarly for zext
========= ===== ===== ===== ======== ============================= =================================
LOCK      4     DST   -     -        lock [DST]                    Expects i32
UNLOCK    4     DST   -     -        lock [DST]                    Expects i32
PRINTF    T     FMT   FMT   FMT      printf [FMT]                  FMT has no %d
PRINTF    T     FMT   SRC   SRC      printf [FMT] T [SRC]          FMT has only one %d
PRINTF    T     FMT   SRC1  SRC2     printf [FMT] T [SRC1] [SRC2]  FMT has 2 %d, both SRCs size T
========= ===== ===== ===== ======== ============================= =================================

