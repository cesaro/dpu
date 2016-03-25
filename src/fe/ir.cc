
#include <cassert>
#include <cstring>
#include <exception>
#include <algorithm>

#include "verbosity.h"
#include "misc.hh"

#include "fe/ir.hh"

using namespace dpu::fe::ir;

Symbol * Module::sym_lookup (const std::string & name)
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

	std::string n (name);
	s = sym_lookup (n);
	if (s != 0)
	{
		// if there was already a symbol with same name, redefine address
		addrtab.erase (s->addr);
		s->addr = addr;
		addrtab[addr] = s;
		s->size = size;
	}
	else
	{
		// if not, create new symbol pointing to the newly allocated memory space
		s = new Symbol (name, addr, size);
		symtab[name] = s;
		addrtab[addr] = s;
		//if (s->name.size () >= MAX_SYM_NAME) throw std::logic_error ("Maximum length for symbol name exceeded");
	}

	return s;
}

void Module::validate ()
{
	// - coherence of memory size and symbols in the symtab
	// - numthreads = threads.
	// - immediate values in instructions do not have enabled bits out of range
	// - addresses in all instructions are within memory ranges, including +size
	// - addresses in all instructions are aligned to data size
	// - all instructions with 2 or more predecessors or a BRxx predecessor have non-empty label
	// - all instructions have expected types 1,2,4,8; except for printf
	// - warn if symbols "overlap"
	// - coherence between labeltab in function and pointed instructions
	// - if instruction has more than 1 successor, then it has 2 and they are brz and brnz
	// - if instruction has 0 successors, then it is error or ret
	// - if i2 in post of i1, then i1 in pre of i2, and vice versa
	// - function's pointer to the module is this module
	// - no instruction operand is a PC (instructions do not modify PCs)
	// - no BRZ / BRNZ instructions are authorized here, only BR
	// - labels and symbols are of the form [_a-zA-Z][_a-zA-Z0-9]*

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
	int i;

	fprintf (f, "Module at %p: ", this);
	fprintf (f, "%zu functions, %zuB memory, %zu symbols, %zd instructions (ever allocated)\n\n",
			functions.size (), memory.size (), symtab.size (), instructions.size ());

	print_symtab (f);
	fputs ("\n", f);

	for (auto fun : functions)
	{
		i = fprintf (f, "Function \"%s\"\n", fun->name.c_str ()) - 1;
		fputs ((std::string ("=") * i).c_str (), f);
		fputs ("\n", f);
		fun->print (f);
		fputs ("End\n===\n\n", f);
	}
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

Instruction * Function::label_lookup (const std::string & label)
{
	auto it = labeltab.find (label);
	if (it == labeltab.end ()) return 0;
	return it->second;
}

void Function::label_set (std::string && label, Instruction * ins)
{
	auto it = labeltab.find (label);
	if (it != labeltab.end ()) it->second->label.clear ();
	ins->label = std::move (label);
	labeltab[ins->label] = ins;
}

void Function::labels_clear ()
{
	labeltab.clear ();
}

void Function::print (FILE * f)
{
	std::vector<Instruction*> stack;
	unsigned m;
	Instruction * ins;

	if (entry == 0)
	{
		fputs ("  ; empty body\n", f);
		return;
	}

	// DFS exploration of the CFG, marking explored instructions
	m = new_mark ();
	stack.push_back (entry);
	while (stack.size ())
	{
		// pop an instruction and explore the leftmost branch from there
		ins = stack.back ();
		stack.pop_back ();
		while (ins and ins->m != m)
		{
			// visit the instruction
			ins->m = m;
			// print label, if present
			if (ins->label.size () != 0) fprintf (f, "%s :\n", ins->label.c_str ());
			// check for unauthorized instructions
			if (ins->op == BRZ or ins->op == BRNZ)
			{
				fputs ("  ; ERROR: found disallowed brz/brnz instruction\n", f);
				return;
			}
			// print the instruction
			fprintf (f, "  %s\n", module->print_instr(ins).c_str());
			// set backtrack point here if there is an "else" branch to explore
			if (ins->op == BR) stack.push_back (ins->br_nextz ());
			ins = ins->next;
		}
	}
}

unsigned Function::new_mark ()
{
	return module->new_mark ();
}

Program::Program (unsigned mt) :
	numthreads (_numthreads),
	_numthreads (mt)
{
   DEBUG ("%p: fe::ir::Program.ctor: numthreads %u", this, numthreads);
}

Function * Program::add_thread (std::string name)
{
	Function * f = module.add_function (name);
	threads.push_back (f);
	return f;
}

void Program::validate ()
{
	// - numthreads = threads.size()
	// - main is one of the threads

	throw std::logic_error ("Not implemented");
}

void Program::print (FILE * f)
{
	fprintf (f, "Program at %p: ", this);
	fprintf (f, "%zu threads, %u numthreads, main thread '%s', module at %p\n",
			threads.size (), numthreads,
			main ? main->name.c_str() : "(null)", &module);
	module.print (f);
}

std::string Module::print_instr (Instr * ins)
{
	// return fmt ("op '%s' size %u dst %u src1 %u src2 %u", op2str (), size, dst, src1, src2);
	switch (ins->op)
	{
	// miscellaneous instructions
	case ERROR :
		return "error";
	case PRINTF :
		return fmt ("printf  %s \"%s\" [%s] [%s]",
				ins->size2str (),
				memory.data () + ins->dst,
				print_addr(ins->src1).c_str (),
				print_addr(ins->src2).c_str ());

	// synchronization instructions and return
	case LOCK :
	case UNLOCK :
		return fmt ("%-7s %s [%s]",
				ins->op2str (),
				ins->size2str (),
				print_addr(ins->dst).c_str ());
	case RET :
		return fmt ("ret     %s [%s]",
				ins->size2str (),
				print_addr(ins->src1).c_str ());
	case RETI :
		return fmt ("ret     %s 0x%lx", ins->size2str (), ins->src2);

	// move instructions
	case MOVE :
		return fmt ("move    %s [%s] [%s]",
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str ());
	case MOVEI :
		return fmt ("move    %s [%s] 0x%lx",
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				ins->src2);
	case IMOV :
		return fmt ("imov    %s [%s] [[%s]]",
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

		return fmt ("%-7s %s [%s] [%s] [%s]",
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
		return fmt ("%-7s %s [%s] [%s] 0x%lx",
				ins->op2str (),
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str (),
				ins->src2);

	//case FDIV :
	//case FREM : return "FIXME";

	// branch instructions
	case BR :
		return fmt ("br      i32 [%s] ??? ???",
				print_addr(ins->src1).c_str ());
	case BRZ :
		return fmt ("brz     i32 [%s] ???",
				print_addr(ins->src1).c_str ());
	case BRNZ :
		return fmt ("brnz    i32 [%s] ???",
				print_addr(ins->src1).c_str ());

	// sign extension instructions
	case SEXT :
	case ZEXT :
		return fmt ("%-7s %s to %s [%s] [%s]",
				ins->op2str (),
				ins->size2str (ins->size),
				ins->size2str (ins->src2),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str ());
	}
}

std::string Module::print_instr (Instruction * ins)
{
	switch (ins->op)
	{
	case BR :
		return fmt ("br i32 [%s] %s %s",
				print_addr (ins->src1).c_str (),
				print_label (ins->br_nextnz ()).c_str (),
				print_label (ins->br_nextz  ()).c_str ());
	case BRZ :
		return fmt ("brz i32 [%s] %s",
				print_addr(ins->src1).c_str (),
				print_label (ins->next).c_str ());
	case BRNZ :
		return fmt ("brnz i32 [%s] %s",
				print_addr(ins->src1).c_str (),
				print_label (ins->next).c_str ());
	default :
		return print_instr ((Instr *) ins);
	}
}

std::string Module::print_label (Instruction * ins)
{
	if (ins == 0) return "ERROR_NULL_PTR";
	if (ins->label.size () == 0) return "ERROR_EMPTY_LABEL";
	return ins->label;
}

const char * Instr::op2str ()
{
	switch (op)
	{
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
	case BR :
		return "br";
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

Builder::Builder (Function * f) :
	f (f),
	last (0),
	last_label (0),
	branch (1) // nz branch
{ }

void Builder::attach (Function * f)
	{ this->f = f; f->entry = 0; last = 0; }

void Builder::attach (Function * f, Instruction * ins)
	{ this->f = f; last = ins; }

void Builder::attach (Instruction * ins)
	{ last = ins; }

Instruction * Builder::get_last ()
	{ return last; }

void Builder::set_branch (int b)
	{ branch = b; }

std::string Builder::gen_label ()
	{ return fmt ("l%d", last_label++); }

void Builder::set_label ()
	{ set_label (last, gen_label ()); }

void Builder::set_label (std::string && label)
	{ set_label (last, std::move (label)); }

void Builder::set_label  (Instruction * ins)
	{ set_label (ins, gen_label ()); }

void Builder::set_label (Instruction * ins, std::string && label)
{
	// disallowed if we didn't first generate one instruction
	if (last == 0)
		throw std::logic_error ("Tried to set a label before adding at least one instruction.");
	f->label_set (std::move(label), ins);
}

void Builder::insert (Instruction *ins)
{
	// put the instruction in the module's set of instructions, for
	// memory-leaking pourposes
	f->module->instructions.push_back (ins);

	// if last is null, this is the first instruction of the function
	if (last == 0)
	{
		if (f->entry != 0) f->labels_clear ();
		f->entry = ins;
		ins->pre.clear ();
	}
	else
	{
		// if last instruction is a BR instruction, then variable branch tells
		// which branch to insert
		if (last->op == BR)
		{
			if (branch)
				last->next = ins; // "then / nz" branch
			else
				last->src2 = (uint64_t) ins; // "else / z" branch
		}
		else
		{
			last->next = ins;
		}
		ins->pre.push_back (last);
	}

	// the new instruction becomes the last one
	last = ins;
}

Instruction * Builder::mk_error ()
{
	Instruction * i = new Instruction ();
	i->op = ERROR;
	insert (i);
	return i;
}

Instruction * Builder::mk_ret   (Datasize s, Addr src)
{
	Instruction * i = new Instruction ();
	i->op = RET;
	i->size = s;
	i->src1 = src;
	insert (i);
	return i;
}

Instruction * Builder::mk_ret   (Datasize s, Imm imm)
{
	Instruction * i = new Instruction ();
	i->op = RETI;
	i->size = s;
	i->src2 = imm;
	insert (i);
	return i;
}

Instruction * Builder::mk_move  (Datasize s, Addr dst, Addr src)
{
	Instruction * i = new Instruction ();
	i->op = MOVE;
	i->size = s;
	i->dst = dst;
	i->src1 = src;
	insert (i);
	return i;
}
Instruction * Builder::mk_move  (Datasize s, Addr dst, Imm imm)
{
	Instruction * i = new Instruction ();
	i->op = MOVEI;
	i->size = s;
	i->dst = dst;
	i->src2 = imm;
	insert (i);
	return i;
}
Instruction * Builder::mk_imov  (Datasize s, Addr dst, Addr src)
{
	Instruction * i = new Instruction ();
	i->op = IMOV;
	i->size = s;
	i->dst = dst;
	i->src1 = src;
	insert (i);
	return i;
}

