
#include <cassert>
#include <cstring>
#include <exception>
#include <algorithm>

#include "verbosity.h"
#include "misc.hh"

#include "fe/ir.hh"

using namespace dpu::fe::ir;

Symbol * Module::sym_lookup (std::string & name)
{
	auto it = symtab.find (name);
	if (it == symtab.end ()) return 0;
	return it->second;
}

Symbol * Module::allocate (const char * name, uint32_t size, uint32_t align,
		uint64_t initval)
{
	return allocate (name, size, align, &initval);
}

Symbol * Module::allocate (const char * name, uint32_t size, uint32_t align)
{
	return allocate (name, size, align, (void *) 0);
}

Symbol * Module::allocate (const char * name, uint32_t size, uint32_t align,
		const void * initval)
{
	uint32_t addr;
	uint32_t rem;
	uint32_t padding;
	uint32_t currsize;
	Symbol * s;

	// compute the address as the current memory size + necessary padding
	currsize = memory.size ();
	rem = currsize % align;
	padding = rem == 0 ? 0 : align - rem;
	addr = currsize + padding;

	DEBUG ("%p: Module::allocate: currsize %zd padding %zd addr %zd", this, currsize, padding, addr);

	// resize the memory container and copy initial value
	memory.resize (addr + size);
	if (initval) memcpy (memory.data () + addr, initval, size);

	// create a symbol pointed to the newly allocated memory space
	s = new Symbol (name, addr, size);
	if (s->name.size () >= MAX_SYM_NAME) throw std::logic_error ("Maximum length for symbol name exceeded");

	// insert the symbol in the symbol table and (reverse) address table
	symtab[name] = s;
	addrtab[addr] = s;

	return s;
}

void Module::validate ()
{
	// - coherence of memory size and symbols in the symtab
	// - maxthreads = threads.
	// - immediate values in instructions do not have enabled bits out of range
	// - addresses in all instructions are within memory ranges, including +size
	// - addresses in all instructions are aligned to data size
	// - all instructions with 2 or more predecessors or a BRxx predecessor have non-empty label
	// - all instructions have expected types 1,2,4,8; except for printf

	throw std::logic_error ("Not implemented");
}

Function * Module::add_function (std::string name)
{
	Function * f = new Function (name, this);
	functions.push_back (f);
	return f;
}

Module::Module () :
	mark (1)
{
   DEBUG ("%p: fe::ir::Module.ctor:", this);
}

Module::~Module ()
{
   DEBUG ("%p: fe::ir::Module.dtor:", this);
	for (auto & p : symtab) delete p.second;
	for (auto & i : instructions) delete i;
}

void Module::print (FILE * f)
{

	fprintf (f, "Module at %p: ", this);
	fprintf (f, "%zu functions, %zuB memory, %zu symbols, %zd instructions (ever allocated)\n",
			functions.size (), memory.size (), symtab.size (), instructions.size ());
	print_symtab (f);
}

std::string Module::print_addr (uint32_t addr)
{
	uint32_t a = addr;
	Symbol * s = 0;

	// this could be done much better ...
	while (true)
	{
		auto it = addrtab.find (a);
		if (it != addrtab.end ())
		{
			s = it->second;
			break;
		}
		if (a == 0) break;
		--a;
	}
	if (s == 0) return fmt ("0x%04x", addr);
	assert (s->addr == a);
	if (a == addr) return fmt ("%s", s->name.c_str ());
	return fmt ("%s+0x%x", s->name.c_str (), addr - s->addr);
}

void Module::print_symtab (FILE * f)
{
#if 0
	// put in a vector and sort by address
	std::vector<Symbol*> symbols;
	for (auto & p : symtab) symbols.push_back (p.second);
	struct {
		bool operator () (Symbol * s1, Symbol * s2) { return s1->addr < s2->addr; }
	} cmp;
	std::sort (symbols.begin(), symbols.end (), cmp);
#endif

	fprintf (f, "Address    Size  Initial                 Symbol\n");
	fprintf (f, "========== ===== ======================= =====================================\n");
	
	for (auto & p : addrtab)
	{
		fprintf (f, "0x%08x %5u ", uint32_t (p.second->addr), p.second->size);
		for (size_t i = 0; i < p.second->size; ++i)
		{
			if (i % 8 == 0 and i != 0) fprintf (f, "\n                 ");
			fprintf (f, "%02x ", memory[p.second->addr + i]);
		}
		if (p.second->size % 8 != 0)
			for (size_t i = p.second->size % 8; i < 8; ++i)
				fprintf (f, "   ");
		fprintf (f, "%s\n", p.second->name.c_str ());
	}
	fprintf (f, "========== ===== ======================= =====================================\n");
}

Function::Function (std::string name, Module * m) :
	name (name),
	module (m),
	entry (0)
{
   DEBUG ("%p: fe::ir::Function.ctor: name '%s' module %p", this, name.c_str(), m);
}

Function::~Function ()
{
   DEBUG ("%p: fe::ir::Module.dtor:", this);
}


Program::Program (unsigned mt) :
	maxthreads (_maxthreads),
	_maxthreads (mt)
{
   DEBUG ("%p: fe::ir::Program.ctor: maxthreads %u", this, maxthreads);
}

Function * Program::add_thread (std::string name)
{
	Function * f = module.add_function (name);
	threads.push_back (f);
	return f;
}

void Program::validate ()
{
	// - maxthreads = threads.size()
	// - main is one of the threads

	throw std::logic_error ("Not implemented");
}

void Program::print (FILE * f)
{
	fprintf (f, "Program at %p: ", this);
	fprintf (f, "%zu threads, %u maxthreads, main thread '%s', module at %p\n",
			threads.size (), maxthreads,
			main ? main->name.c_str() : "(null)", &module);
	module.print (f);
}

std::string Module::print_instr (Instr * ins)
{
	// return fmt ("op '%s' size %u dst %u src1 %u src2 %u", op2str (), size, dst, src1, src2);
	switch (ins->op)
	{
	// miscellaneous instructions
	case NOP :
		return "nop";
	case ERROR :
		return "error";
	case PRINTF :
		return fmt ("printf %s \"%s\" [%s] [%s]",
				ins->size2str (),
				memory.data () + ins->dst,
				print_addr(ins->src1).c_str (),
				print_addr(ins->src2).c_str ());

	// synchronization instructions and return
	case LOCK :
	case UNLOCK :
		return fmt ("%s %s [%s]",
				ins->op2str (),
				ins->size2str (),
				print_addr(ins->dst).c_str ());
	case RET :
		return fmt ("ret %s [%s]",
				ins->size2str (),
				print_addr(ins->src1).c_str ());
	case RETI :
		return fmt ("ret %s 0x%lx", ins->size2str (), ins->src2);

	// move instructions
	case MOVE :
		return fmt ("move %s [%s] [%s]",
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str ());
	case MOVEI :
		return fmt ("move %s [%s] 0x%lx",
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				ins->src2);
	case IMOV :
		return fmt ("imov %s [%s] [[%s]]",
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str ());

	// dst = src1 op src2
	case CMP_EQ :
	case CMP_NE :
	case CMP_UGT :
	case CMP_UGE :
	case CMP_ULT :
	case CMP_ULE :
	case CMP_SGT :
	case CMP_SGE :
	case CMP_SLT :
	case CMP_SLE :
	case ADD :
	case SUB :
	case MUL :
	case SDIV :
	case UDIV :
	case SREM :
	case UREM :
	case OR :
	case AND :
	case XOR :

		return fmt ("%s %s [%s] [%s] [%s]",
				ins->op2str (),
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str (),
				print_addr(ins->src2).c_str ());

	// dst = src1 op imm
	case CMP_EQI :
	case CMP_NEI :
	case CMP_UGTI :
	case CMP_UGEI :
	case CMP_ULTI :
	case CMP_ULEI :
	case CMP_SGTI :
	case CMP_SGEI :
	case CMP_SLTI :
	case CMP_SLEI :
	case ADDI :
	case SUBI :
	case MULI :
	case SDIVI :
	case UDIVI :
	case SREMI :
	case UREMI :
	case ORI :
	case ANDI :
	case XORI :
		return fmt ("%s %s [%s] [%s] 0x%lx",
				ins->op2str (),
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str (),
				ins->src2);

	//case FDIV :
	//case FREM : return "FIXME";

	// branch instructions
	case BRZ : return "brz";
		return fmt ("brz i32 [%s] l%lu",
				print_addr(ins->src1).c_str (),
				ins->src2);
	case BRNZ : return "brnz";
		return fmt ("brnz i32 [%s] l%lu",
				print_addr(ins->src1).c_str (),
				ins->src2);

	// sign extension instructions
	case SEXT :
	case ZEXT :
		return fmt ("%s %s to %s [%s] [%s]",
				ins->op2str (),
				ins->size2str (ins->size),
				ins->size2str (ins->src2),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str ());
	}
}

const char * Instr::op2str ()
{
	switch (op)
	{
	case NOP :
		return "nop";
	case ERROR :
		return "error";
	case RET :
	case RETI :
		return "ret";

	//
	case MOVE :
	case MOVEI :
		return "move";
	case IMOV :
		return "imov";

	//
	case CMP_EQ :
	case CMP_EQI :
		return "cmp eq";
	case CMP_NE :
	case CMP_NEI :
		return "cmp ne";
	case CMP_UGT :
	case CMP_UGTI :
		return "cmp ugt";
	case CMP_UGE :
	case CMP_UGEI :
		return "cmp uge";
	case CMP_ULT :
	case CMP_ULTI :
		return "cmp ult";
	case CMP_ULE :
	case CMP_ULEI :
		return "cmp ule";
	case CMP_SGT :
	case CMP_SGTI :
		return "cmp sgt";
	case CMP_SGE :
	case CMP_SGEI :
		return "cmp sge";
	case CMP_SLT :
	case CMP_SLTI :
		return "cmp slt";
	case CMP_SLE :
	case CMP_SLEI :
		return "cmp sle";

	//
	case BRZ :
		return "brz";
	case BRNZ :
		return "brnz";

	//
	case ADD :
	case ADDI :
		return "add";
	case SUB :
	case SUBI :
		return "sub";
	case MUL :
	case MULI :
		return "mul";

	case SDIV :
	case SDIVI :
		return "sdiv";
	case UDIV :
	case UDIVI :
		return "udiv";
	case SREM :
	case SREMI :
		return "srem";
	case UREM :
	case UREMI :
		return "urem";

	//case FDIV :
	//case FREM : return "FIXME";

	case OR :
	case ORI :
		return "or";
	case AND :
	case ANDI :
		return "and";
	case XOR :
	case XORI :
		return "xor";

	case SEXT : return "sext";
	case ZEXT : return "zext";

	case LOCK : return "lock";
	case UNLOCK : return "unlock";
	case PRINTF : return "printf";
	}
}

const char * Instr::size2str (uint32_t s)
{
	static char buff[16]; // FIXME, non-thread safe
	switch (s)
	{
	case 1 : return "i8";
	case 2 : return "i16";
	case 4 : return "i32";
	case 8 : return "i64";
	default :
		sprintf (buff, "i%d", size * 8);
		return buff;
	}
}

uint64_t Instr::cast_val (uint64_t v)
{
	switch (size)
	{
	case 1 : return uint8_t (v);
	case 2 : return uint16_t (v);
	case 4 : return uint32_t (v);
	case 8 : return v;
	default :
		uint64_t mask = 0;
		for (unsigned i = 0; i < size; ++i)
		{
			mask <<= 8;
			mask |= 0xff;
		}
		return v & mask;
	}
}
