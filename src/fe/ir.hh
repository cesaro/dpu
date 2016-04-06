
#ifndef __DPU_FE_IR_HH_
#define __DPU_FE_IR_HH_

#include <cstdio>
#include <map>
#include <vector>

#include "verbosity.h"

namespace dpu {
namespace fe {
namespace ir {

struct Addr
{
	uint32_t addr;
	Addr (uint32_t a) : addr (a) {};
	operator uint32_t () const { return addr; }
};

struct Imm
{
	uint64_t imm;
	Imm (uint32_t i) : imm (i) {};
	operator uint64_t () { return imm; }
};

struct Symbol
{
public :
	std::string  name;
	Addr			 addr;
	uint32_t     size;

	Symbol (std::string && n, Addr a, uint32_t s) :
		name (n), addr (a), size (s)
		{ DEBUG ("%p: fe::ir::Symbol.ctor: name '%s' size %u, addr %p", this, name.c_str(), s, addr.addr); }

	Symbol (const char * n, Addr a, uint32_t s) :
		Symbol (std::string (n), a, s) {}

	~Symbol () { DEBUG ("%p: fe::ir::Symbol.dtor: name '%s' addr %p", this, name.c_str(), addr.addr); }

	operator std::string () { return name; }
	operator const char * () { return name.c_str (); }
	operator Addr () { return addr; }
};

enum Opcode
{
	// exit instructions
	ERROR,
	RET,
	RETI,
	// direct and indirect move
	MOVE,
	MOVEI,
	MOVIS,
	MOVID,
   // cmp xxx DST SRC1 SRC2
	CMP_EQ,
	CMP_NE,
	CMP_UGT,
	CMP_UGE,
	CMP_ULT,
	CMP_ULE,
	CMP_SGT,
	CMP_SGE,
	CMP_SLT,
	CMP_SLE,
   // cmp xxx DST SRC1 IMM
	CMP_EQI,
	CMP_NEI,
	CMP_UGTI,
	CMP_UGEI,
	CMP_ULTI,
	CMP_ULEI,
	CMP_SGTI,
	CMP_SGEI,
	CMP_SLTI,
	CMP_SLEI,
	// branch
	BR,
	BRZ,
	BRNZ,
   // arithmetic
	ADD,
	ADDI,
	SUB,
	SUBI,
	MUL,
	MULI,
	SDIV,
	SDIVIA,
	SDIVAI,
	UDIV,
	UDIVIA,
	UDIVAI,
	SREM,
	SREMIA,
	SREMAI,
	UREM,
	UREMIA,
	UREMAI,
	//FDIV,
	//FREM,
	// or DST SRC1 SRC2
	OR,
	AND,
	XOR,
	// or DST SRC1 IMM32
	ORI,
	ANDI,
	XORI,
	// sign extension
	SEXT,
	ZEXT,
	// miscellaneous instructions
	LOCK,
	UNLOCK,
	PRINTF
};

enum Datasize
{
	I8  = 1,
	I16 = 2,
	I32 = 4,
	I64 = 8,
	FLOAT = 4,
	DOUBLE = 8,
};

struct Instr
{
	uint32_t dst;
	uint32_t src1;
	uint64_t src2;
	Opcode   op : 16;
	uint16_t size;

	const char * op2str ();
	const char * size2str (uint32_t s);
	const char * size2str () { return size2str (size); }
	uint64_t     cast_val (uint64_t v);
	uint64_t     cast_imm () { return cast_val (src2); }
	bool         has_imm ();
   void         exec (uint8_t * mem);
};

struct Instruction : Instr
{
	std::vector<Instruction*> pre;
	Instruction *             next;
	unsigned                  m;
	std::string               label;
	std::string               comment;

	// ctor, default dtor
	Instruction () :
		next (0), m (0) {}

	void set_next  (Instruction * ins);
	void set_nextz (Instruction * ins);
	void rm_next   ();
	void rm_nextz  ();

	Instruction * get_next   () { return next; }
	Instruction * get_nextnz () { return next; }
	Instruction * get_nextz  () { return op == BR ? (Instruction *) src2 : 0; }
};

class Module;
class Function
{
public :
	std::string   name;
	Module *      module;
	Instruction * entry;

	Function (std::string name, Module * m);
	~Function ();

	Instruction * label_lookup (const std::string & label);
	void          label_set    (std::string && label, Instruction * ins);
	void          labels_clear ();
	void          print        (FILE * f);
	void          print2       (FILE * f);
	void          dump         () { print (stderr); }
	void          dump2        () { print2 (stderr); }
	unsigned      new_mark     ();

private :
	std::map<std::string, Instruction*> labeltab;

	friend class Module;
};

class Module
{
public :
	// ctor and dtor
	Module ();
	~Module ();

	// allocating symbols and getting their value
	Symbol *    sym_lookup   (const std::string & name);
	const uint8_t * sym_init_val (const Symbol * s);
	Symbol *    allocate     (const char * name, uint32_t size, uint32_t align, uint64_t initval);
	Symbol *    allocate     (const char * name, uint32_t size, uint32_t align, const void * initval);
	Symbol *    allocate     (const char * name, uint32_t size, uint32_t align); // initial value = zero
	Function *  add_function (std::string name);

	// printing functions, instructions, etc.
	std::string print_instr  (Instruction * ins) const;
	std::string print_instr  (Instr * ins) const;
	std::string print_addr   (uint32_t addr) const;
	std::string print_label  (Instruction * ins) const;
	void        print_symtab (FILE * f) const;
	void        print        (FILE * f) const;
	void        dump         () const { print (stderr); }

	// miscellanea
	void        validate     (uint32_t data_start = 0);
	unsigned    new_mark     () { return mark++; }

	// enumerators
	typedef       std::vector<Function*>::iterator     iterator;
	typedef       std::vector<Instruction*>::iterator  inst_iterator;
	typedef       std::vector<Symbol*>::iterator       sym_iterator;
	iterator      begin        () { return functions.begin (); }
	iterator      end          () { return functions.end (); }
	inst_iterator inst_begin   () { return instructions.begin (); }
	inst_iterator inst_end     () { return instructions.end (); }
	sym_iterator  sym_begin    () { return symbols.begin (); }
	sym_iterator  sym_end      () { return symbols.end (); }

private :
	std::vector<uint8_t>           memory;
	std::vector<Function*>         functions;
	std::vector<Instruction*>      instructions;
	std::vector<Symbol*>           symbols;
	std::map<std::string, Symbol*> symtab;
	std::map<uint32_t, Symbol*>    addrtab;
	unsigned                       mark;

	void __validate_2c_2d_2e (Instruction *i, uint32_t min, uint32_t max) const;

	friend class Builder;
};

class Program
{
public :
	// module, threads, main function
	Module 						module;
	std::vector<Function*>  threads;
	Function *					main;
	const unsigned &        numthreads;

	// ctor takes as argument final number of threads that will be added
	Program (unsigned numthreads);
	Function * add_thread (std::string name);

	void validate ();

	void print (FILE * f);
	void dump () { print (stderr); }

private :
	unsigned _numthreads;
};

class Builder
{
public :
	Builder (Function * f);

	void          attach     (Function * f);
	void          attach     (Function * f, Instruction * ins);
	void          attach     (Instruction * ins);
	Instruction * get_last   ();
	void          set_branch (int b); // 1 for nz branch; 0 for z branch

	void          push       ();
	void          push       (Instruction * ins);
	Instruction * pop        ();
	Instruction * peek       ();

	std::string   gen_label  ();
	void          set_label  ();
	void          set_label  (std::string && label);
	void          set_label  (Instruction * ins);
	void          set_label  (Instruction * ins, std::string && label);

	void          set_comment (const std::string & s);
	void          set_comment (std::string && s);
	void          set_comment (Instruction * ins, std::string && s);

	// error and return
	Instruction * mk_error   ();
	Instruction * mk_ret     (Datasize s, Addr src);
	Instruction * mk_ret     (Datasize s, Imm imm);

	// moving data
	Instruction * mk_move    (Datasize s, Addr dst, Addr src);
	Instruction * mk_move    (Datasize s, Addr dst, Imm imm);
	Instruction * mk_movis   (Datasize s, Addr dst, Addr src);
	Instruction * mk_movid   (Datasize s, Addr dst, Addr src);

	// compare instructions
	Instruction * mk_cmp_eq  (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_eq  (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_ne  (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_ne  (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_ugt (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_ugt (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_uge (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_uge (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_ult (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_ult (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_ule (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_ule (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_sgt (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_sgt (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_sge (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_sge (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_slt (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_slt (Datasize s, Addr dst, Addr src1, Imm imm);
	Instruction * mk_cmp_sle (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_cmp_sle (Datasize s, Addr dst, Addr src1, Imm imm);

	// branch
	Instruction * mk_br      (Addr src, Instruction * nzbr, Instruction * zbr);
	Instruction * mk_br      (Addr src);

	// arithmetic instructions
	Instruction * mk_add     (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_add     (Datasize s, Addr dst, Addr src, Imm imm);
	Instruction * mk_sub     (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_sub     (Datasize s, Addr dst, Imm imm, Addr src);
	Instruction * mk_mul     (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_mul     (Datasize s, Addr dst, Addr src, Imm imm);
	Instruction * mk_sdiv    (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_sdiv    (Datasize s, Addr dst, Imm imm, Addr src);
	Instruction * mk_sdiv    (Datasize s, Addr dst, Addr src, Imm imm);
	Instruction * mk_udiv    (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_udiv    (Datasize s, Addr dst, Imm imm, Addr src);
	Instruction * mk_udiv    (Datasize s, Addr dst, Addr src, Imm imm);
	Instruction * mk_srem    (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_srem    (Datasize s, Addr dst, Imm imm, Addr src);
	Instruction * mk_srem    (Datasize s, Addr dst, Addr src, Imm imm);
	Instruction * mk_urem    (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_urem    (Datasize s, Addr dst, Imm imm, Addr src);
	Instruction * mk_urem    (Datasize s, Addr dst, Addr src, Imm imm);

	// bitwise logic instructions
	Instruction * mk_or      (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_or      (Datasize s, Addr dst, Addr src, Imm imm);
	Instruction * mk_and     (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_and     (Datasize s, Addr dst, Addr src, Imm imm);
	Instruction * mk_xor     (Datasize s, Addr dst, Addr src1, Addr src2);
	Instruction * mk_xor     (Datasize s, Addr dst, Addr src, Imm imm);

	// sign extension
	Instruction * mk_sext    (Datasize from, Datasize to, Addr dst, Addr src);
	Instruction * mk_zext    (Datasize from, Datasize to, Addr dst, Addr src);

	// mutex and printf
	Instruction * mk_lock    (Addr addr);
	Instruction * mk_unlock  (Addr addr);
	Instruction * mk_printf  (Addr fmt, Datasize s, Addr src1, Addr src2);
	Instruction * mk_printf  (const char * fmt);
	Instruction * mk_printf  (const char * fmt, Datasize s, Addr src1);
	Instruction * mk_printf  (const char * fmt, Datasize s, Addr src1, Addr src2);

private :
	Function *                f;
	Instruction *             last;
	unsigned                  last_label;
	int                       branch;
	std::vector<Instruction*> stack;

	void insert (Instruction *ins);
};

} } } // namespace dpu::fe::ir

#endif

