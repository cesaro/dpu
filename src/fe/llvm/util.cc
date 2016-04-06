#include <iostream>
#include <cstring>
#include <iomanip>
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"

#include "util.tpp"
#include "util.hh"

namespace dpu {
namespace fe {
namespace llvm {

/* Compares two function names:
 * s1 is a hard-defined name
 * s2 is a name obtained from the IR
 * Returns 0 if the names are the same, something else otherwise
 */

int safeCompareFname( const char* s1, std::string s2 ){
    int l1, l2;
    
    l1 = std::strlen( s1 );
    l2 = s2.size();

    return (l1==l2)?(std::strcmp( s1, s2.c_str() )):-1;
}

/* Prints the contents of a block
 * (used for debugging purpose)
 */

void dumpBlock( llvm::BasicBlock* bb ) {
    for( auto ins = bb->begin() ; ins != bb->end() ; ins++ ) {
        ins->print( llvm::errs() );
        std::cerr << "\n";
    }
}

/* Returns the list of names that are not functions of the second list
 */

std::vector<llvm::Function*> notInList( std::vector<llvm::Function*> funclist, std::vector<std::string> nameslist ) {
    std::vector<llvm::Function*> notInList;
    for( auto fun = funclist.begin() ; fun != funclist.end() ; fun++ ) {
        std::string n = (*fun)->getName().str();
        if( ! itemexists( nameslist, n ) ) {
            notInList.push_back( *fun );
        }
    }
    return notInList;
}

/* Get an (unsorted) list of blocks used by a function
 */

std::vector<llvm::BasicBlock*> blocklist( llvm::Function* fun ) {
    llvm::BasicBlock* current;
    llvm::TerminatorInst *term;
    int numsucc;
    llvm::BasicBlock* next;
    
    llvm::BasicBlock* entryBlock = fun->begin();
    std::vector<llvm::BasicBlock*> visitedblocks;
    std::vector<llvm::BasicBlock*> nextblocks;
        
    /* Get the blocks in a list. The order does not matter here */
    
    visitedblocks.push_back( entryBlock );
    nextblocks.push_back( entryBlock );

    do{
        current = nextblocks.back();
        nextblocks.pop_back();
        term = current->getTerminator();
        numsucc = term->getNumSuccessors();

        for( int i = 0 ; i < numsucc ; i++ ) {
            next = term->getSuccessor( i );
            if( ! itemexists( visitedblocks, next ) ) {
                nextblocks.push_back( next );
            } 
            visitedblocks.push_back( next );
        }
    } while( !nextblocks.empty() );
    
    return visitedblocks;
}

/* http://www.cs.cmu.edu/~15745/assignment2/Dataflow/available-support.cpp
 */

std::string getShortValueName( llvm::Value * v ) {
    if( v->hasName() &&  v->getName().str().length() > 0) {
        return "%" + v->getName().str();
    }
    else{
        if (llvm::isa<llvm::Instruction>(v)) {
            std::string s = "";
				llvm::raw_string_ostream * strm = new llvm::raw_string_ostream(s);
            v->print(*strm);
            std::string inst = strm->str();
            size_t idx1 = inst.find("%");
            size_t idx2 = inst.find(" ",idx1);
            if (idx1 != std::string::npos && idx2 != std::string::npos) {
                return inst.substr(idx1,idx2-idx1);
            }
            else {
                return "\"" + inst + "\"";
            }
        } 
        else{
            llvm::ConstantInt *cint = llvm::dyn_cast<llvm::ConstantInt>(v);
            if( cint ) {
                std::string s = "";
					 llvm::raw_string_ostream * strm = new llvm::raw_string_ostream(s);
                cint->getValue().print(*strm,true);
            return strm->str();
            } else {
                std::string s = "";
					 llvm::raw_string_ostream * strm = new llvm::raw_string_ostream(s);
                v->print(*strm);
                std::string inst = strm->str();
                return "\"" + inst + "\"";
            }
        }
    }
}

std::vector<std::string> getOperands( llvm::Instruction* ins ) {
    std::vector<std::string> names;
    //    std::cerr << "Instruction: " ; ins->print( errs() ); std::cerr << std::endl;
    for( auto op = ins->op_begin() ; op != ins->op_end() ; op++ ) {
        llvm::Value* val = op->get();
        std::string name;
        switch( ins->getOpcode() ) { /*  TODO: needs to be completed wit other operations */
		  case opcodes.LoadInst:
        case opcodes.CallInst:
        case opcodes.AllocaInst: // case when no name in this case: %1 %2 %3 allocated for argc and argv
        case opcodes.ICmpInst:
        case opcodes.StoreInst:
            name = (val->hasName()) ? val->getName().str() : getShortValueName( val );
            break;
        default:
            name = "";
            break;
        }
        if( name != "" ) {
            //            std::cerr << name << std::endl;
            names.push_back( name );
        }
    }
    return names;    
}

/* Is the current variable used later?
 * I couln't find the liveness analysis API such as LiveValues. 
 * Maybe my installation is too old (3.5). So I implemented a quick
 * pass to see it the value stored here is used later.
 */

bool isLive( std::string name, llvm::Function* fun  ){

    /* Take each and every instruction of the function */
    
    std::vector<llvm::BasicBlock*> visitedblocks = blocklist( fun ) ;
    for( auto bb = visitedblocks.begin() ; bb != visitedblocks.end() ; bb++ ) {
        for( auto i = (*bb)->begin() ; i != (*bb)->end() ; i++ ) {

            std::vector<std::string> opnames = getOperands( &(*i) );
            if( itemexists( opnames, name ) ) {
                return true;
            } 
        }
    }
    return false;
}

/* Get what alloca allocates
 * Returns two integers:
 * - size in bits of each field
 * - number of blocks
 *
 * FIXME -- arrays of more than 1 dimension !!!
 *
 */

std::pair<unsigned, unsigned> getAllocaInfo( llvm::Instruction* ins ) {
    std::pair<unsigned, unsigned> info;
    
    llvm::AllocaInst* aa = static_cast<llvm::AllocaInst*>(&(*ins));
    
    if( aa->getAllocatedType()->isPointerTy() ) {
        /* Pointers give a size of 0. Use something else instead.*/
        info.first = POINTERSIZE;
    } else {
        info.first = aa->getAllocatedType()->getPrimitiveSizeInBits();
    }

    llvm::Value* size = aa->getArraySize();
    std::string s_size =  (size->hasName()) ? size->getName().str() : getShortValueName( size );
    info.second = std::stoi( s_size );
    
    return info;
}

/* Dump the state of a machine
 * Used for debugging
 */

void  dumpMachine(  machine_t* machine ){
    std::cerr << "Address \t| Symbol / threadid \t| Value" << std::endl;
    
    for( auto i = machine->begin() ; i != machine->end() ; i++ ) {
        if( i->first.first != "" ){
            std::cerr << i->second << "\t\t|" << i->first.first << " / " << i->first.second << "\t\t";
            if( 0 == i->first.second && i->first.first.length( ) < 3 )
                std::cerr << "\t";
            std::cerr << "|" << " TODO " << std::endl;
        }
    }
}

/* Is this variable a global variable? 
 */

/* TODO : support backward check to support uggly stuff like
static int a;
int foo( int t ){
	int* p;
	if( t > 3 ) {
		p = &a;
	} else {
		p = &t;
	}
	return *p;
}
*/


bool isGlobal( llvm::Module* mod, llvm::StringRef name ){
	llvm::GlobalVariable* var = mod->getGlobalVariable( name );
	return (var == NULL ) ? false:true;
}

/* Puts the type of a value into a string 
 */

std::string typeToStr( llvm::Type* ty ) {
    std::string s;
    /* Type */
    if( ty->isIntegerTy() ) {
        s = "i";
    }
    if( ty->isFloatingPointTy() ) {
        s = "f";
    }
    /* Length */
    if( !ty->isPointerTy() ) {
        s = s + std::to_string( ty->getPrimitiveSizeInBits() );
    } else {
        s = "i" + std::to_string( POINTERSIZE );
    }
    return s;
}

/* Pretty print, can be modified, that gives the line number etc
 * Prints, in three colunms;
 * - the line number
 * - the block
 * - the thread id
 */

void printLineNb( int line, llvm::BasicBlock* block, llvm::Value* tid ) {
    std::cout << std::setfill(' ') << std::setw(3) << line << " " << block << " ";
    if( NULL == tid ) {
        std::cout << "main      | ";
    } else {
            std::cout << tid << " | ";
    }
}

} } } // namespace dpu::fe::llvm

