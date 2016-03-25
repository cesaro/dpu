
#ifndef __DPU_FE_IR_HH_
#define __DPU_FE_IR_HH_

#include <cstdio>
#include <map>
#include <vector>

#include "verbosity.h"

namespace dpu {
namespace fe {
namespace ir {

const int MAX_SYM_NAME = 256;
const int MAX_INST_TEXT = MAX_SYM_NAME * 4;

struct Addr
{
	uint32_t addr;
	Addr (uint32_t a) : addr (a) {};
	operator uint32_t () { return addr; }
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
		name (n), addr (a), size (s) {}

	Symbol (const char * n, Addr a, uint32_t s) :
		Symbol (std::string (n), a, s) {}

	~Symbol () { DEBUG ("%p: fe::ir::Symbol.dtor: name '%s' addr %p", this, name.c_str(), addr.addr); }

	operator std::string () { return name; }
	operator const char * () { return name.c_str (); }
	operator Addr () { return addr; }
};

enum Opcode
{
	ERROR,
	RET,
	RETI,
	//
	MOVE,
	MOVEI,
	IMOV,
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
	//
	BR,
	BRZ,
	BRNZ,
   // add DST SRC1 SRC2
	ADD,
	SUB,
	MUL,
	SDIV,
	UDIV,
	SREM,
	UREM,
   // add DST SRC1 IMM
	ADDI,
	SUBI,
	MULI,
	SDIVI,
	UDIVI,
	SREMI,
	UREMI,
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
	//
	SEXT,
	ZEXT,
	//
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
};

struct Instruction : Instr
{
	std::vector<Instruction*> pre;
	Instruction *             next;
	unsigned                  m;
	std::string               label;

	Instruction * br_nextnz () { return next; }
	Instruction * br_nextz  () { return (Instruction *) src2; }
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
	unsigned      new_mark     ();

private :
	std::map<std::string, Instruction*> labeltab;
};

class Module
{
public :
	Module ();
	~Module ();

	Symbol *    sym_lookup   (const std::string & name);
	Symbol *    allocate     (const char * name, uint32_t size, uint32_t align, uint64_t initval);
	Symbol *    allocate     (const char * name, uint32_t size, uint32_t align, const void * initval);
	Symbol *    allocate     (const char * name, uint32_t size, uint32_t align); // initial value = zero
	Function *  add_function (std::string name);
	void        validate     ();
	std::string print_instr  (Instruction * ins);
	std::string print_instr  (Instr * ins);
	std::string print_addr   (uint32_t addr);
	std::string print_label  (Instruction * ins);
	void        print        (FILE * f);
	void        dump         () { print (stderr); }
	unsigned    new_mark     () { return mark++; }

private :
	std::vector<unsigned char>     memory;
	std::map<std::string, Symbol*> symtab;
	std::map<uint32_t, Symbol*>    addrtab;
	std::vector<Function*>         functions;
	std::vector<Instruction*>      instructions;
	unsigned                       mark;

	void        print_symtab (FILE * f);

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

	Instruction * push       ();
	Instruction * push       (Instruction * ins);
	Instruction * pop        ();

	std::string   gen_label  ();
	void          set_label  ();
	void          set_label  (std::string && label);
	void          set_label  (Instruction * ins);
	void          set_label  (Instruction * ins, std::string && label);

	Instruction * mk_error ();
	Instruction * mk_ret   (Datasize s, Addr src);
	Instruction * mk_ret   (Datasize s, Imm imm);
	Instruction * mk_move  (Datasize s, Addr dst, Addr src);
	Instruction * mk_move  (Datasize s, Addr dst, Imm imm);
	Instruction * mk_imov  (Datasize s, Addr dst, Addr src);

private :
	Function *    f;
	Instruction * last;
	unsigned      last_label;
	int           branch;

	void insert (Instruction *ins);
};

} } } // namespace dpu::fe::ir

#endif

