
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

Adressing Modes and types
---------------

- Immediate operands, given directly on the instruction
- Absolute/Direct access: address of the data to access is given in the
  instruction operand
- Indirect access: only the instruction ``imov`` is able to perform indirect
  access to data

Types:

- i8, i16, i32, i64
- float, double

Operation Codes, Syntax and "Semantics"
---------------------------------------

T   = Type
DST = Destination address (address)
SRC = Source address or immediate value (address, integer, or float)
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
cmp ult T DST SRC1 SRC2     Comparison: unsigned less than
cmp ule T DST SRC1 SRC2     Comparison: unsigned less or equal
cmp slt T DST SRC1 SRC2     Comparison: signed less than
cmp sle T DST SRC1 SRC2     Comparison: signed less or equal
br        LAB               Unconditional branch
br        SRC LAB1 LAB2     Conditional branch
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
or      T DST SRC1 SRC2     Logical or; expects integers
and     T DST SRC1 SRC2     Logical and; expects integers
xor     T DST SRC1 SRC2     Logical xor; expects integers
=========================== ====================================================
sext    T1 T2 DST SRC       Signed extension from type T1 to T2; expects integer
zext    T1 T2 DST SRC       Zero extends SRC from type T1 to T2; expects integer
=========================== ====================================================
lock    T SRC               Locks a mutex; expects address of integer operand
unlock  T SRC               Unlocks a mutex; expects address of integer operand
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

Data Structures
---------------

TBD
