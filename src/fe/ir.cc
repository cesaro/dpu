
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

const uint8_t * Module::sym_init_val (const Symbol * s)
{
	if (uint32_t (s->addr) >= memory.size ()) return 0;
	return memory.data () + s->addr;
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

	DEBUG ("%p: Module::allocate: currsize %zd padding %zd addr %zd name '%s'",
			this, currsize, padding, addr, name);

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
		symbols.push_back (s);
	}

	return s;
}

void Module::validate (uint32_t data_start)
{
	// 1.   coherence of memory size and symbols in the symtab
	// 2a.  all instructions have expected types 1,2,4,8; sext/zext also
	// 2b.  immediate values in instructions do not have enabled bits out of range
	// 2c.  addresses in all instructions are within memory ranges, including +size
	// 2d.  instructions do not read or write PCs
	// 2e.  addresses in all instructions are aligned to instruction's data size
	// 3a.  if instruction has 0 successors, then it is error or ret
	// 3b.  no BRZ / BRNZ instructions are authorized in this IR, only BR
	// 3c.  both branches of a BR instruction point to some other instruction (no end)
	// 3d.  if i2 is next of i1, then i1 in pre of i2
	// 3e.  if i1 in pre of i2, then i2 is next of i1
	// 3f.  all instructions with 2 or more predecessors or a BRxx predecessor have non-empty label
	// 4.   coherence between labeltab in function and pointed instructions
	// 5.   function's pointer to the module is this module
	//
	// 6.   label names and symbol names are of the form [_a-zA-Z][_a-zA-Z0-9]*
	// 7.   warn if symbols "overlap"

	// currently 6 and 7 are not enforced

	size_t memsize = memory.size ();

	for (auto & p : symtab)
	{
		// checking 1
		if (p.second->addr + p.second->size > memsize)
		{
			std::string s = fmt ("Symbol '%s', addr %lu, size %lu: out of memory range (%zu bytes)",
					p.second->name.c_str (), p.second->addr, p.second->size, memsize);
			throw std::logic_error (s);
		}
	}

	for (auto i : instructions)
	{
		// checking 2a
		if (i->size != 1 and i->size != 2 and i->size != 4 and i->size != 8)
		{
			std::string s = fmt ("'%s': invalid operand size",
					print_instr (i).c_str());
			throw std::logic_error (s);
		}
		if (i->op == SEXT or i->op == ZEXT)
		{
			if ((i->src2 != 2 and i->src2 != 4 and i->src2 != 8) or
					i->size >= i->src2)
			{
				std::string s = fmt ("'%s': invalid target operand size",
						print_instr (i).c_str());
				throw std::logic_error (s);
			}
		}

		// checking 2b
		if (i->has_imm () and i->cast_imm () != i->src2)
		{
			std::string s = fmt ("'%s': immediate operand: high bits beyond operand size",
					print_instr (i).c_str());
			throw std::logic_error (s);
		}

		// checking 2c
		// checking 2d
		// checking 2e
		__validate_2c_2d_2e (i, data_start, memsize);

		// checking 3a
		if (i->next == 0 and i->op != ERROR and i->op != RET and i->op != RETI)
		{
			std::string s = fmt ("'%s': has no next instruction but is not 'error' or 'ret'",
					print_instr (i).c_str());
			throw std::logic_error (s);
		}

		// checking 3b
		if (i->op == BRZ or i->op == BRNZ)
		{
			std::string s = fmt ("'%s': disallowed instruction brz or brnz",
					print_instr (i).c_str());
			throw std::logic_error (s);
		}

		// checking 3c (only the z branch, the nz one was already checked)
		if (i->op == BR and i->src2 == 0)
		{
			std::string s = fmt ("'%s': the 'else' branch has no next instruction",
					print_instr (i).c_str());
			throw std::logic_error (s);
		}

		// checking 3d
		// checking 3e
		if (i->next and
				i->next->pre.end() == std::find (i->next->pre.begin(), i->next->pre.end(), i))
		{
			std::string s = fmt ("'%s': invalid 'next/pre' pointers detected, bug! (check with Function::dump2 ;)",
					print_instr (i).c_str());
			throw std::logic_error (s);
		}
		if (i->op == BR and i->get_nextz ())
		{
			Instruction * n = i->get_nextz ();
			if (n->pre.end() == std::find (n->pre.begin(), n->pre.end(), i))
			{
				std::string s = fmt ("'%s': invalid 'next/pre' pointers detected, bug! (check with Function::dump2 ;)",
						print_instr (i).c_str());
				throw std::logic_error (s);
			}
		}

		for (auto ii : i->pre)
		{
			if (not (ii->next == i or (ii->op == BR and ii->get_nextz() == i)))
			{
				std::string s = fmt ("'%s': invalid 'next/pre' pointers detected, bug! (check with Function::dump2 ;)",
						print_instr (i).c_str());
				throw std::logic_error (s);
			}
		}

		// checking 3f
		if (i->pre.size () >= 2 and i->label.size () == 0)
		{
			std::string s = fmt ("'%s': has >= 2 predecessor basic blocks but has no label",
					print_instr (i).c_str());
			throw std::logic_error (s);
		}

	}

	// checking 4
	unsigned m = new_mark ();
	for (auto f : functions)
	{
		for (auto & p : f->labeltab)
		{
			p.second->m = m;
			if (p.first != p.second->label)
			{
				std::string s = fmt ("'%s': data structure courruption detected: unexpected label",
						print_instr (p.second).c_str());
				throw std::logic_error (s);
			}
		}
		// checking 5
		if (f->module != this)
		{
			std::string s = fmt ("Function '%s': unexpected pointer to Module object",
					f->name.c_str ());
			throw std::logic_error (s);
		}
	}
	for (auto i : instructions)
	{
		if (i->label.size () and i->m !=m)
		{
			std::string s = fmt ("'%s': data structure courruption detected: unexpected label",
					print_instr (i).c_str());
			throw std::logic_error (s);
		}
	}
}

void Module::__validate_2c_2d_2e (Instruction *i, uint32_t min, uint32_t max) const
{
#define CHECK_RANGE(a,s) \
	{ \
		if ((a) < min or ((a) +(s)) > max) \
		{ \
			throw std::logic_error (fmt ("'%s': invalid address: beyond memory range", print_instr (i).c_str())); \
		} \
	}
#define CHECK_ALIGN(a,s) \
	{ \
		if ((a) % (s) != 0) \
		{ \
			throw std::logic_error (fmt ("'%s': address misaligned to operand size", print_instr (i).c_str())); \
		} \
	}

	switch (i->op)
	{
	case ERROR :
	case RETI :
		// 0 address to check
		return;

	// checking only src1
	case RET :
	case BR :
	case BRZ :
	case BRNZ :
		CHECK_RANGE (i->src1, i->size);
		CHECK_ALIGN (i->src1, i->size);
		return;

	// checking only dst
	case MOVEI :
	case LOCK :
	case UNLOCK :
		CHECK_RANGE (i->dst, i->size);
		CHECK_ALIGN (i->dst, i->size);
		return;

	// checking: dst, src1
	case MOVE :
	case MOVID :
	case MOVIS :
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
	case SDIVIA :
	case SDIVAI :
	case UDIVIA :
	case UDIVAI :
	case SREMIA :
	case SREMAI :
	case UREMIA :
	case UREMAI :
	case ORI :
	case ANDI :
	case XORI :
		CHECK_RANGE (i->dst, i->size);
		CHECK_ALIGN (i->dst, i->size);
		CHECK_RANGE (i->src1, i->size);
		CHECK_ALIGN (i->src1, i->size);
		return;

	// checking: dst, src1, src2
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
	//case FDIV :
	//case FREM :
	case OR :
	case AND :
	case XOR :
		CHECK_RANGE (i->dst, i->size);
		CHECK_ALIGN (i->dst, i->size);
		CHECK_RANGE (i->src1, i->size);
		CHECK_ALIGN (i->src1, i->size);
		CHECK_RANGE (i->src2, i->size);
		CHECK_ALIGN (i->src2, i->size);
		return;

	// special case: dst, src1, with size being src2
	case SEXT :
	case ZEXT :
		CHECK_RANGE (i->dst, i->src2);
		CHECK_ALIGN (i->dst, i->src2);
		CHECK_RANGE (i->src1, i->src2);
		CHECK_ALIGN (i->src1, i->src2);
		return;

	// special case: src1, src2
	case PRINTF :
		CHECK_RANGE (i->src1, i->size);
		CHECK_ALIGN (i->src1, i->size);
		CHECK_RANGE (i->src2, i->size);
		CHECK_ALIGN (i->src2, i->size);
		return;
	}
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
	for (auto i : instructions) delete i;
	for (auto f : functions) delete f;
}

Function * Module::add_function (std::string name)
{
	Function * f = new Function (name, this);
	functions.push_back (f);
	return f;
}

void Module::print (FILE * f) const
{
	int i;

	fprintf (f, "Module at %p: ", this);
	fprintf (f, "%zu functions, %zuB memory, %zu symbols, %zd instructions (including unreachable)\n\n",
			functions.size (), memory.size (), symtab.size (), instructions.size ());

	print_symtab (f);
	fputs ("\n", f);

	for (auto fun : functions)
	{
		i = fprintf (f, "Function at %p \"%s\"\n", fun, fun->name.c_str ()) - 1;
		fputs ((std::string ("=") * i).c_str (), f);
		fputs ("\n", f);
		fun->print (f);
		fputs ("End\n===\n\n", f);
	}
}

std::string Module::print_addr (uint32_t addr) const
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

void Module::print_symtab (FILE * f) const
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
   DEBUG ("%p: fe::ir::Function.dtor:", this);
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

void Function::print2 (FILE * f)
{
	std::vector<Instruction*> stack;
	unsigned m;
	Instruction * ins;
	int count = 0;

	fputs ("==========\n", f);
	if (entry == 0) { fputs ("(empty body)\n", f); return; }

	// DFS exploration of the CFG, marking explored instructions
	m = new_mark ();
	stack.push_back (entry);
	while (stack.size ())
	{
		ins = stack.back ();
		stack.pop_back ();
		while (ins and ins->m != m)
		{
			ins->m = m;
			count++;
			fprintf (f, "%p: %s\n   label: '%s'\n   next: %p pre: ",
					ins,  module->print_instr (ins).c_str (),
					ins->label.c_str (),
					ins->next);
			if (ins->pre.size () == 0) fputs ("(empty)", f);
			for (auto & inss : ins->pre) fprintf (f, "%p, ", inss);
			fputs ("\n\n", f);
			if (ins->op == BR) stack.push_back (ins->get_nextz ());
			ins = ins->get_next ();
		}
	}
	fprintf (f, "Done: %d instructions\n", count);
	fputs ("==========\n", f);
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
		while (ins)
		{
			// if the instruction is already visited, print "pseudo branch"
			if (ins->m == m)
			{
				if (ins->label.size () == 0)
				{
					fputs ("  ; ERROR: found unlabelled basic block which is target of 2 instructions", f);
					return;
				}
				fprintf (f, "  br      %s\n\n", ins->label.c_str ());
				break;
			}
			// visit the instruction
			ins->m = m;
			// print label, if present
			if (ins->label.size () != 0) fprintf (f, "%s :\n", ins->label.c_str ());
			// print comment, if present
			if (ins->comment.size () != 0) fprintf (f, "; %s\n", ins->comment.c_str ());
			// check for unauthorized instructions
			if (ins->op == BRZ or ins->op == BRNZ)
			{
				fputs ("  ; ERROR: found disallowed brz/brnz instruction\n", f);
				return;
			}
			// print the instruction
			fprintf (f, "  %s\n", module->print_instr(ins).c_str());
			// set backtrack point here if there is an "else" branch to explore
			if (ins->op == BR)
			{
				stack.push_back (ins->get_nextz ());
				fputs ("\n", f);
			}
			ins = ins->next;
		}
	}
}

unsigned Function::new_mark ()
{
	return module->new_mark ();
}

Program::Program (unsigned mt) :
	main (0), // null
	numthreads (_numthreads),
	_numthreads (mt)
{
	char buff[32];
   DEBUG ("%p: fe::ir::Program.ctor: numthreads %u", this, numthreads);

	if (mt == 0)
		throw std::logic_error ("Number of threads must be greater than 0");

	// allocate memory space for program counters
	for (unsigned i = 0; i < mt; ++i)
	{
		sprintf (buff, "__pc%u", i);
		module.allocate (buff, 4, 4);
	}
}

Function * Program::add_thread (std::string name)
{
	Function * f = module.add_function (name);
	threads.push_back (f);
	return f;
}

void Program::validate ()
{
	// 1.   numthreads = threads.
	// 2.   main is one of the functions in the module

	// checking 1
	if (numthreads != threads.size ())
	{
		std::string s = fmt ("Program declared with %d threads but only %d were defined",
				numthreads, threads.size ());
		throw std::logic_error (s);
	}

	// checking 2
	if (main == 0)
		throw std::logic_error ("Program has no main thread set");
	auto it = std::find (module.begin(), module.end(), main);
	if (it == module.end ())
		throw std::logic_error ("The main thread is not one of the functions of the Module");

	// validate the module
	module.validate (numthreads * 4);
}

void Program::print (FILE * f)
{
	fprintf (f, "Program at %p: ", this);
	fprintf (f, "%zu threads, %u numthreads, main thread '%s', module at %p\n",
			threads.size (), numthreads,
			main ? main->name.c_str() : "(null)", &module);
	module.print (f);
}

std::string Module::print_instr (Instr * ins) const
{
	// return fmt ("op '%s' size %u dst %u src1 %u src2 %u", op2str (), size, dst, src1, src2);
	switch (ins->op)
	{
	// miscellaneous instructions
	case ERROR :
		return "error";
	case PRINTF :
		return fmt ("printf  \"%s\" %s [%s] [%s]",
				quoted_str ((const char *) memory.data () + ins->dst).c_str (),
				ins->size2str (),
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
	case MOVIS :
		return fmt ("imov    %s [%s] [[%s]]",
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str ());
	case MOVID :
		return fmt ("imov    %s [[%s]] [%s]",
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
	case MULI :
	case SDIVAI :
	case UDIVAI :
	case SREMAI :
	case UREMAI :
	case ORI :
	case ANDI :
	case XORI :
		return fmt ("%-7s %s [%s] [%s] 0x%lx",
				ins->op2str (),
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				print_addr(ins->src1).c_str (),
				ins->src2);

	// dst = imm op src1
	case SUBI :
	case SDIVIA :
	case UDIVIA :
	case SREMIA :
	case UREMIA :
		return fmt ("%-7s %s [%s] 0x%lx [%s]",
				ins->op2str (),
				ins->size2str (),
				print_addr(ins->dst).c_str (),
				ins->src2,
				print_addr(ins->src1).c_str ());

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

std::string Module::print_instr (Instruction * ins) const
{
	switch (ins->op)
	{
	case BR :
		return fmt ("br      i32 [%s] %s %s",
				print_addr (ins->src1).c_str (),
				print_label (ins->get_nextnz ()).c_str (),
				print_label (ins->get_nextz  ()).c_str ());
	case BRZ :
		return fmt ("brz     i32 [%s] %s",
				print_addr(ins->src1).c_str (),
				print_label (ins->next).c_str ());
	case BRNZ :
		return fmt ("brnz    i32 [%s] %s",
				print_addr(ins->src1).c_str (),
				print_label (ins->next).c_str ());
	default :
		return print_instr ((Instr *) ins);
	}
}

std::string Module::print_label (Instruction * ins) const
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
	case MOVIS :
		return "movis";
	case MOVID :
		return "movid";

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
	case SDIVIA :
	case SDIVAI :
		return "sdiv";
	case UDIV :
	case UDIVIA :
	case UDIVAI :
		return "udiv";
	case SREM :
	case SREMIA :
	case SREMAI :
		return "srem";
	case UREM :
	case UREMIA :
	case UREMAI :
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

bool Instr::has_imm ()
{
	switch (op)
	{
	case ERROR :
	case RET :
	case MOVE :
	case MOVIS :
	case MOVID :
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
	case BR :
	case BRZ :
	case BRNZ :
	case ADD :
	case SUB :
	case MUL :
	case SDIV :
	case UDIV :
	case SREM :
	case UREM :
	//case FDIV :
	//case FREM :
	case OR :
	case AND :
	case XOR :
	case SEXT :
	case ZEXT :
	case LOCK :
	case UNLOCK :
	case PRINTF :
		return false;

	case RETI :
	case MOVEI :
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
	case SDIVIA :
	case SDIVAI :
	case UDIVIA :
	case UDIVAI :
	case SREMIA :
	case SREMAI :
	case UREMIA :
	case UREMAI :
	case ORI :
	case ANDI :
	case XORI :
		return true;
	}
}

void Instruction::set_next (Instruction * ins)
{
	rm_next ();
	next = ins;
	ins->pre.push_back (this);
}

void Instruction::rm_next ()
{
	if (next == 0) return;
	auto it = std::find (next->pre.begin(), next->pre.end(), this);
	if (it == next->pre.end ())
		throw std::logic_error ("Found instruction with faulty next/pre pointers");
	next->pre.erase (it);
	next = 0;
}

void Instruction::set_nextz (Instruction * ins)
{
	if (op != BR)
		throw std::logic_error ("Call to set_nextz on instruciton other than BR");
	rm_nextz ();
	src2 = (uint64_t) ins;
	ins->pre.push_back (this);
}

void Instruction::rm_nextz ()
{
	if (op != BR)
		throw std::logic_error ("Call to set_nextz on instruciton other than BR");
	if (src2 == 0) return;
	Instruction * n = (Instruction *) src2;
	auto it = std::find (n->pre.begin(), n->pre.end(), this);
	if (it == n->pre.end ())
		throw std::logic_error ("Found BR instruction with faulty next/pre pointers");
	n->pre.erase (it);
	src2 = 0;
}

Builder::Builder (Function * f) :
	f (f),
	last (0),
	last_label (0),
	branch (1), // nz branch
	stack ()
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

void Builder::push ()
	{ push (last); }

void Builder::push (Instruction * ins)
	{ stack.push_back (ins); }

Instruction * Builder::pop  ()
	{ auto i = stack.back(); stack.pop_back(); last = i; return i; }

Instruction * Builder::peek  ()
	{ return stack.back(); }

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
		throw std::logic_error ("Cannot set a label before adding the first instruction.");
	f->label_set (std::move(label), ins);
}

void Builder::set_comment (const std::string & s)
	{ set_comment (last, std::string (s)); }

void Builder::set_comment (std::string && s)
	{ set_comment (last, std::move (s)); }

void Builder::set_comment (Instruction * ins, std::string && s)
{
	// disallowed if we didn't first generate one instruction
	if (ins == 0)
		throw std::logic_error ("Cannot set a comment before adding the first instruction.");
	if (ins->comment.size () != 0)
		ins->comment.append ("\n; ");
	ins->comment += std::move (s);
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
	}
	else
	{
		// if last instruction is a BR instruction, then variable branch tells
		// which branch to insert
		if (last->op == BR)
		{
			if (branch)
				last->set_next (ins); // "then / nz" branch
			else
				last->set_nextz (ins); // "else / z" branch
		}
		else
		{
			last->set_next (ins);
		}
	}

	// the new instruction becomes the last one
	last = ins;
}

#define INSTR_MACRO1(o,s,d,s1,s2) \
	Instruction * i = new Instruction (); \
	i->op   = o; \
	i->size = s; \
	i->dst  = d; \
	i->src1 = s1; \
	i->src2 = s2; \
	insert (i)

#define INSTR_MACRO2(op,s,dst,src1,src2) \
	INSTR_MACRO1 (op, s, dst, src1, src2); \
	return i


Instruction * Builder::mk_error ()
{
	INSTR_MACRO2 (ERROR, 1, 0, 0, 0);
}

Instruction * Builder::mk_ret   (Datasize s, Addr src)
{
	INSTR_MACRO2 (RET, s, 0, src, 0);
}

Instruction * Builder::mk_ret   (Datasize s, Imm imm)
{
	INSTR_MACRO2 (RETI, s, 0, 0, imm);
}

Instruction * Builder::mk_move  (Datasize s, Addr dst, Addr src)
{
	INSTR_MACRO2 (MOVE, s, dst, src, 0);
}

Instruction * Builder::mk_move  (Datasize s, Addr dst, Imm imm)
{
	INSTR_MACRO2 (MOVEI, s, dst, 0, imm);
}

Instruction * Builder::mk_movis (Datasize s, Addr dst, Addr src)
{
	INSTR_MACRO2 (MOVIS, s, dst, src, 0);
}

Instruction * Builder::mk_movid (Datasize s, Addr dst, Addr src)
{
	INSTR_MACRO2 (MOVID, s, dst, src, 0);
}

Instruction * Builder::mk_cmp_eq  (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_EQ,  s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_eq  (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_EQI,  s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_ne  (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_NE,  s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_ne  (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_NEI,  s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_ugt (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_UGT, s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_ugt (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_UGTI, s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_uge (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_UGE, s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_uge (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_UGEI, s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_ult (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_ULT, s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_ult (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_ULTI, s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_ule (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_ULE, s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_ule (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_ULEI, s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_sgt (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_SGT, s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_sgt (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_SGTI, s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_sge (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_SGE, s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_sge (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_SGEI, s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_slt (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_SLT, s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_slt (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_SLTI, s, dst, src1, imm);
}

Instruction * Builder::mk_cmp_sle (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (CMP_SLE, s, dst, src1, src2);
}

Instruction * Builder::mk_cmp_sle (Datasize s, Addr dst, Addr src1, Imm imm)
{
	INSTR_MACRO2 (CMP_SLEI, s, dst, src1, imm);
}

Instruction * Builder::mk_br      (Addr src, Instruction * nzbr, Instruction * zbr)
{
	INSTR_MACRO1 (BR, I32, 0, src, 0);
	if (zbr) i->set_nextz (zbr);
	if (nzbr) i->set_next (nzbr);
	return i;
}

Instruction * Builder::mk_br      (Addr src)
{
	return mk_br (src, 0, 0);
}

Instruction * Builder::mk_add     (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (ADD , s, dst, src1, src2);
}

Instruction * Builder::mk_add     (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (ADDI  , s, dst, src, imm);
}

Instruction * Builder::mk_sub     (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (SUB , s, dst, src1, src2);
}

Instruction * Builder::mk_sub     (Datasize s, Addr dst, Imm imm, Addr src)
{
	INSTR_MACRO2 (SUBI  , s, dst, src, imm);
}

Instruction * Builder::mk_mul     (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (MUL , s, dst, src1, src2);
}

Instruction * Builder::mk_mul     (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (MULI  , s, dst, src, imm);
}

Instruction * Builder::mk_sdiv    (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (SDIV, s, dst, src1, src2);
}

Instruction * Builder::mk_sdiv    (Datasize s, Addr dst, Imm imm, Addr src)
{
	INSTR_MACRO2 (SDIVIA, s, dst, src, imm);
}

Instruction * Builder::mk_sdiv    (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (SDIVAI, s, dst, src, imm);
}

Instruction * Builder::mk_udiv    (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (UDIV, s, dst, src1, src2);
}

Instruction * Builder::mk_udiv    (Datasize s, Addr dst, Imm imm, Addr src)
{
	INSTR_MACRO2 (UDIVIA, s, dst, src, imm);
}

Instruction * Builder::mk_udiv    (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (UDIVAI, s, dst, src, imm);
}

Instruction * Builder::mk_srem    (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (SREM, s, dst, src1, src2);
}

Instruction * Builder::mk_srem    (Datasize s, Addr dst, Imm imm, Addr src)
{
	INSTR_MACRO2 (SREMIA, s, dst, src, imm);
}

Instruction * Builder::mk_srem    (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (SREMAI, s, dst, src, imm);
}

Instruction * Builder::mk_urem    (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (UREM, s, dst, src1, src2);
}

Instruction * Builder::mk_urem    (Datasize s, Addr dst, Imm imm, Addr src)
{
	INSTR_MACRO2 (UREMIA, s, dst, src, imm);
}

Instruction * Builder::mk_urem    (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (UREMAI, s, dst, src, imm);
}

Instruction * Builder::mk_or      (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (OR, s, dst, src1, src2);
}

Instruction * Builder::mk_or      (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (ORI , s, dst, src, imm);
}

Instruction * Builder::mk_and     (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (AND, s, dst, src1, src2);
}

Instruction * Builder::mk_and     (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (ANDI, s, dst, src, imm);
}

Instruction * Builder::mk_xor     (Datasize s, Addr dst, Addr src1, Addr src2)
{
	INSTR_MACRO2 (XOR, s, dst, src1, src2);
}

Instruction * Builder::mk_xor     (Datasize s, Addr dst, Addr src, Imm imm)
{
	INSTR_MACRO2 (XORI, s, dst, src, imm);
}

Instruction * Builder::mk_sext    (Datasize from, Datasize to, Addr dst, Addr src)
{
	INSTR_MACRO2 (SEXT, from, dst, src, to);
}

Instruction * Builder::mk_zext    (Datasize from, Datasize to, Addr dst, Addr src)
{
	INSTR_MACRO2 (ZEXT, from, dst, src, to);
}

Instruction * Builder::mk_lock    (Addr addr)
{
	INSTR_MACRO2 (LOCK, I32, addr, 0, 0);
}

Instruction * Builder::mk_unlock  (Addr addr)
{
	INSTR_MACRO2 (UNLOCK, I32, addr, 0, 0);
}

Instruction * Builder::mk_printf  (Addr fmt, Datasize s, Addr src1, Addr src2)
{
	INSTR_MACRO2 (PRINTF, s, fmt, src1, src2);
}

Instruction * Builder::mk_printf  (const char * fmt_, Datasize s, Addr src1, Addr src2)
{
	std::string symname = fmt ("__fmt%u", last_label++);
	Symbol * sym = f->module->allocate (symname.c_str(), strlen (fmt_) + 1, 1, fmt_);
	if (src1 == 0)
		return mk_printf (sym->addr, I8, *sym, *sym);
	if (src2 == 0)
		return mk_printf (sym->addr, s, src1, src1);
	return mk_printf (sym->addr, s, src1, src2);
}

Instruction * Builder::mk_printf  (const char * fmt)
{
	return mk_printf (fmt, I8, 0, 0);
}

Instruction * Builder::mk_printf  (const char * fmt, Datasize s, Addr src1)
{
	return mk_printf (fmt, s, src1, 0);
}


void Instr::exec (uint8_t * mem)
{
	uint32_t addr;

	switch (op)
	{
	case ERROR :
	case RET :
	case RETI :
	case BR :
	case BRZ :
	case BRNZ :
		return;

	//
	case MOVE :
		switch (size)
		{
		case 1 : * ( uint8_t *) (mem + dst) = * ( uint8_t *) (mem + src1); return;
		case 2 : * (uint16_t *) (mem + dst) = * (uint16_t *) (mem + src1); return;
		case 4 : * (uint32_t *) (mem + dst) = * (uint32_t *) (mem + src1); return;
		case 8 : * (uint64_t *) (mem + dst) = * (uint64_t *) (mem + src1); return;
		default : return;
		}

	case MOVEI :
		switch (size)
		{
		case 1 : * ( uint8_t *) (mem + dst) = ( uint8_t) (src2); return;
		case 2 : * (uint16_t *) (mem + dst) = (uint16_t) (src2); return;
		case 4 : * (uint32_t *) (mem + dst) = (uint32_t) (src2); return;
		case 8 : * (uint64_t *) (mem + dst) = (uint64_t) (src2); return;
		default : return;
		}

	case MOVIS :
		addr = * (uint32_t*) (mem + src1);
		// a trash pointer should have been detected at this point
		switch (size)
		{
		case 1 : * ( uint8_t *) (mem + dst) = * ( uint8_t*) (mem + addr); return;
		case 2 : * (uint16_t *) (mem + dst) = * (uint16_t*) (mem + addr); return;
		case 4 : * (uint32_t *) (mem + dst) = * (uint32_t*) (mem + addr); return;
		case 8 : * (uint64_t *) (mem + dst) = * (uint64_t*) (mem + addr); return;
		default : return;
		}

	case MOVID :
		addr = * (uint32_t*) (mem + dst);
		// a trash pointer should have been detected at this point
		switch (size)
		{
		case 1 : * ( uint8_t*) (mem + addr) = * ( uint8_t*) (mem + src1); return;
		case 2 : * (uint16_t*) (mem + addr) = * (uint16_t*) (mem + src1); return;
		case 4 : * (uint32_t*) (mem + addr) = * (uint32_t*) (mem + src1); return;
		case 8 : * (uint64_t*) (mem + addr) = * (uint64_t*) (mem + src1); return;
		default : return;
		}

	case CMP_EQ :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) == *( uint8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) == *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) == *(uint32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) == *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case CMP_EQI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) == ( uint8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) == (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) == (uint32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) == (uint64_t)(src2); return;
		default : return;
		}

	case CMP_NE :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) != *( uint8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) != *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) != *(uint32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) != *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case CMP_NEI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) != ( uint8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) != (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) != (uint32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) != (uint64_t)(src2); return;
		default : return;
		}

	case CMP_UGT :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) > *( uint8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) > *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) > *(uint32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) > *(uint64_t*)(mem+src2); return;
		default : return;
		}
	case CMP_UGTI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) > ( uint8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) > (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) > (uint32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) > (uint64_t)(src2); return;
		default : return;
		}

	case CMP_UGE :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) >= *( uint8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) >= *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) >= *(uint32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) >= *(uint64_t*)(mem+src2); return;
		default : return;
		}
	case CMP_UGEI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) >= ( uint8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) >= (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) >= (uint32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) >= (uint64_t)(src2); return;
		default : return;
		}

	case CMP_ULT :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) < *( uint8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) < *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) < *(uint32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) < *(uint64_t*)(mem+src2); return;
		default : return;
		}
	case CMP_ULTI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) < ( uint8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) < (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) < (uint32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) < (uint64_t)(src2); return;
		default : return;
		}

	case CMP_ULE :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) <= *( uint8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) <= *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) <= *(uint32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) <= *(uint64_t*)(mem+src2); return;
		default : return;
		}
	case CMP_ULEI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( uint8_t*)(mem+src1) <= ( uint8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1) <= (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) <= (uint32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(uint64_t*)(mem+src1) <= (uint64_t)(src2); return;
		default : return;
		}

	case CMP_SGT :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( int8_t*)(mem+src1) > *( int8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(int16_t*)(mem+src1) > *(int16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(int32_t*)(mem+src1) > *(int32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(int64_t*)(mem+src1) > *(int64_t*)(mem+src2); return;
		default : return;
		}
	case CMP_SGTI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( int8_t*)(mem+src1) > ( int8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(int16_t*)(mem+src1) > (int16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(int32_t*)(mem+src1) > (int32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(int64_t*)(mem+src1) > (int64_t)(src2); return;
		default : return;
		}

	case CMP_SGE :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( int8_t*)(mem+src1) >= *( int8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(int16_t*)(mem+src1) >= *(int16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(int32_t*)(mem+src1) >= *(int32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(int64_t*)(mem+src1) >= *(int64_t*)(mem+src2); return;
		default : return;
		}
	case CMP_SGEI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( int8_t*)(mem+src1) >= ( int8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(int16_t*)(mem+src1) >= (int16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(int32_t*)(mem+src1) >= (int32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(int64_t*)(mem+src1) >= (int64_t)(src2); return;
		default : return;
		}

	case CMP_SLT :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( int8_t*)(mem+src1) < *( int8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(int16_t*)(mem+src1) < *(int16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(int32_t*)(mem+src1) < *(int32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(int64_t*)(mem+src1) < *(int64_t*)(mem+src2); return;
		default : return;
		}
	case CMP_SLTI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( int8_t*)(mem+src1) < ( int8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(int16_t*)(mem+src1) < (int16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(int32_t*)(mem+src1) < (int32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(int64_t*)(mem+src1) < (int64_t)(src2); return;
		default : return;
		}

	case CMP_SLE :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( int8_t*)(mem+src1) <= *( int8_t*)(mem+src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(int16_t*)(mem+src1) <= *(int16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(int32_t*)(mem+src1) <= *(int32_t*)(mem+src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(int64_t*)(mem+src1) <= *(int64_t*)(mem+src2); return;
		default : return;
		}
	case CMP_SLEI :
		switch (size)
		{
		case 1 : *(uint32_t*)(mem+dst) = *( int8_t*)(mem+src1) <= ( int8_t)(src2); return;
		case 2 : *(uint32_t*)(mem+dst) = *(int16_t*)(mem+src1) <= (int16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(int32_t*)(mem+src1) <= (int32_t)(src2); return;
		case 8 : *(uint32_t*)(mem+dst) = *(int64_t*)(mem+src1) <= (int64_t)(src2); return;
		default : return;
		}

	// arithmetic

	case ADD :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) + *( uint8_t*)(mem+src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) + *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) + *(uint32_t*)(mem+src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) + *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case ADDI :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) + ( uint8_t)(src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) + (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) + (uint32_t)(src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) + (uint64_t)(src2); return;
		default : return;
		}

	case SUB :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) - *( uint8_t*)(mem+src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) - *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) - *(uint32_t*)(mem+src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) - *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case SUBI :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = ( uint8_t)(src2) - *( uint8_t*)(mem+src1) ; return;
		case 2 : *(uint16_t*)(mem+dst) = (uint16_t)(src2) - *(uint16_t*)(mem+src1) ; return;
		case 4 : *(uint32_t*)(mem+dst) = (uint32_t)(src2) - *(uint32_t*)(mem+src1) ; return;
		case 8 : *(uint64_t*)(mem+dst) = (uint64_t)(src2) - *(uint64_t*)(mem+src1) ; return;
		default : return;
		}

	case MUL :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) * *( uint8_t*)(mem+src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) * *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) * *(uint32_t*)(mem+src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) * *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case MULI :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) * ( uint8_t)(src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) * (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) * (uint32_t)(src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) * (uint64_t)(src2); return;
		default : return;
		}

	case SDIV :
		switch (size)
		{
		case 1 : *( int8_t*)(mem+dst) = *( int8_t*)(mem+src1) / *( int8_t*)(mem+src2); return;
		case 2 : *(int16_t*)(mem+dst) = *(int16_t*)(mem+src1) / *(int16_t*)(mem+src2); return;
		case 4 : *(int32_t*)(mem+dst) = *(int32_t*)(mem+src1) / *(int32_t*)(mem+src2); return;
		case 8 : *(int64_t*)(mem+dst) = *(int64_t*)(mem+src1) / *(int64_t*)(mem+src2); return;
		default : return;
		}

	case SDIVIA :
		switch (size)
		{
		case 1 : *( int8_t*)(mem+dst) = ( int8_t)(src2) / *( int8_t*)(mem+src1); return;
		case 2 : *(int16_t*)(mem+dst) = (int16_t)(src2) / *(int16_t*)(mem+src1); return;
		case 4 : *(int32_t*)(mem+dst) = (int32_t)(src2) / *(int32_t*)(mem+src1); return;
		case 8 : *(int64_t*)(mem+dst) = (int64_t)(src2) / *(int64_t*)(mem+src1); return;
		default : return;
		}

	case SDIVAI :
		switch (size)
		{
		case 1 : *( int8_t*)(mem+dst) = *( int8_t*)(mem+src1) / ( int8_t)(src2); return;
		case 2 : *(int16_t*)(mem+dst) = *(int16_t*)(mem+src1) / (int16_t)(src2); return;
		case 4 : *(int32_t*)(mem+dst) = *(int32_t*)(mem+src1) / (int32_t)(src2); return;
		case 8 : *(int64_t*)(mem+dst) = *(int64_t*)(mem+src1) / (int64_t)(src2); return;
		default : return;
		}

	case UDIV :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) / *( uint8_t*)(mem+src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) / *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) / *(uint32_t*)(mem+src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) / *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case UDIVIA :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = ( uint8_t)(src2) / *( uint8_t*)(mem+src1); return;
		case 2 : *(uint16_t*)(mem+dst) = (uint16_t)(src2) / *(uint16_t*)(mem+src1); return;
		case 4 : *(uint32_t*)(mem+dst) = (uint32_t)(src2) / *(uint32_t*)(mem+src1); return;
		case 8 : *(uint64_t*)(mem+dst) = (uint64_t)(src2) / *(uint64_t*)(mem+src1); return;
		default : return;
		}

	case UDIVAI :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) / ( uint8_t)(src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) / (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) / (uint32_t)(src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) / (uint64_t)(src2); return;
		default : return;
		}

	case SREM :
		switch (size)
		{
		case 1 : *( int8_t*)(mem+dst) = *( int8_t*)(mem+src1) % *( int8_t*)(mem+src2); return;
		case 2 : *(int16_t*)(mem+dst) = *(int16_t*)(mem+src1) % *(int16_t*)(mem+src2); return;
		case 4 : *(int32_t*)(mem+dst) = *(int32_t*)(mem+src1) % *(int32_t*)(mem+src2); return;
		case 8 : *(int64_t*)(mem+dst) = *(int64_t*)(mem+src1) % *(int64_t*)(mem+src2); return;
		default : return;
		}

	case SREMIA :
		switch (size)
		{
		case 1 : *( int8_t*)(mem+dst) = ( int8_t)(src2) % *( int8_t*)(mem+src1); return;
		case 2 : *(int16_t*)(mem+dst) = (int16_t)(src2) % *(int16_t*)(mem+src1); return;
		case 4 : *(int32_t*)(mem+dst) = (int32_t)(src2) % *(int32_t*)(mem+src1); return;
		case 8 : *(int64_t*)(mem+dst) = (int64_t)(src2) % *(int64_t*)(mem+src1); return;
		default : return;
		}

	case SREMAI :
		switch (size)
		{
		case 1 : *( int8_t*)(mem+dst) = *( int8_t*)(mem+src1) % ( int8_t)(src2); return;
		case 2 : *(int16_t*)(mem+dst) = *(int16_t*)(mem+src1) % (int16_t)(src2); return;
		case 4 : *(int32_t*)(mem+dst) = *(int32_t*)(mem+src1) % (int32_t)(src2); return;
		case 8 : *(int64_t*)(mem+dst) = *(int64_t*)(mem+src1) % (int64_t)(src2); return;
		default : return;
		}

	case UREM :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) % *( uint8_t*)(mem+src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) % *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) % *(uint32_t*)(mem+src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) % *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case UREMIA :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = ( uint8_t)(src2) % *( uint8_t*)(mem+src1); return;
		case 2 : *(uint16_t*)(mem+dst) = (uint16_t)(src2) % *(uint16_t*)(mem+src1); return;
		case 4 : *(uint32_t*)(mem+dst) = (uint32_t)(src2) % *(uint32_t*)(mem+src1); return;
		case 8 : *(uint64_t*)(mem+dst) = (uint64_t)(src2) % *(uint64_t*)(mem+src1); return;
		default : return;
		}

	case UREMAI :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) % ( uint8_t)(src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) % (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) % (uint32_t)(src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) % (uint64_t)(src2); return;
		default : return;
		}


	//case FDIV :
	//case FREM : return "FIXME";

	case OR :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) | *( uint8_t*)(mem+src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) | *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) | *(uint32_t*)(mem+src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) | *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case ORI :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) | ( uint8_t)(src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) | (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) | (uint32_t)(src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) | (uint64_t)(src2); return;
		default : return;
		}

	case AND :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) & *( uint8_t*)(mem+src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) & *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) & *(uint32_t*)(mem+src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) & *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case ANDI :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) & ( uint8_t)(src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) & (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) & (uint32_t)(src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) & (uint64_t)(src2); return;
		default : return;
		}

	case XOR :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) ^ *( uint8_t*)(mem+src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) ^ *(uint16_t*)(mem+src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) ^ *(uint32_t*)(mem+src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) ^ *(uint64_t*)(mem+src2); return;
		default : return;
		}

	case XORI :
		switch (size)
		{
		case 1 : *( uint8_t*)(mem+dst) = *( uint8_t*)(mem+src1) ^ ( uint8_t)(src2); return;
		case 2 : *(uint16_t*)(mem+dst) = *(uint16_t*)(mem+src1) ^ (uint16_t)(src2); return;
		case 4 : *(uint32_t*)(mem+dst) = *(uint32_t*)(mem+src1) ^ (uint32_t)(src2); return;
		case 8 : *(uint64_t*)(mem+dst) = *(uint64_t*)(mem+src1) ^ (uint64_t)(src2); return;
		default : return;
		}

	case SEXT :
		switch (src2)
		{
		case 2 :
			*(int16_t*)(mem+dst) = *(int8_t*)(mem+src1);
			return;
		case 4 :
			if (size & 1)
				*(int32_t*)(mem+dst) = *(int8_t*)(mem+src1);
			else
				*(int32_t*)(mem+dst) = *(int16_t*)(mem+src1);
			return;
		case 8 :
			if (size & 1)
			{
				*(int64_t*)(mem+dst) = *(int8_t*)(mem+src1);
			}
			else if (size & 2)
			{
				*(int64_t*)(mem+dst) = *(int16_t*)(mem+src1);
			}
			else
			{
				*(int64_t*)(mem+dst) = *(int32_t*)(mem+src1);
			}
			return;
		default :
			return;
		}

	case ZEXT :
		switch (src2)
		{
		case 2 :
			*(uint16_t*)(mem+dst) = *(uint8_t*)(mem+src1);
			return;
		case 4 :
			if (size & 1)
				*(uint32_t*)(mem+dst) = *(uint8_t*)(mem+src1);
			else
				*(uint32_t*)(mem+dst) = *(uint16_t*)(mem+src1);
			return;
		case 8 :
			if (size & 1)
			{
				*(uint64_t*)(mem+dst) = *(uint8_t*)(mem+src1);
			}
			else if (size & 2)
			{
				*(uint64_t*)(mem+dst) = *(uint16_t*)(mem+src1);
			}
			else
			{
				*(uint64_t*)(mem+dst) = *(uint32_t*)(mem+src1);
			}
			return;
		default :
			return;
		}


	case LOCK :
		// lock is locked iff mem[dst] is true
		if (* (uint32_t*) (mem+dst)) return;
		* (uint32_t*) (mem+dst) = 1;
		return;
	case UNLOCK :
		* (uint32_t*) (mem+dst) = 0;
		return;

	case PRINTF :
		switch (size)
		{
		case 1 :
			printf ((char*)mem+dst, *( uint8_t*)(mem+src1), *( uint8_t*)(mem+src2));
			return;
		case 2 :
			printf ((char*)mem+dst, *(uint16_t*)(mem+src1), *(uint16_t*)(mem+src2));
			return;
		case 4 :
			printf ((char*)mem+dst, *(uint32_t*)(mem+src1), *(uint32_t*)(mem+src2));
			return;
		case 8 :
			printf ((char*)mem+dst, *(uint64_t*)(mem+src1), *(uint64_t*)(mem+src2));
			return;
		default :
			return;
		}
	}
}
