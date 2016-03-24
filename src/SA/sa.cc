#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/CFG.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/Instructions.h"

#include "sa_utils.h"
#include "sa_utils.tpp"

/** 
 * - get the names of all the global variables
 * - get the name of the function called when threads are created
 * - start analyzing the code at the beginning of this function
 **/

using namespace std;
using namespace llvm;

#define PTHREADCREATE "pthread_create"
#define PTHREADJOIN   "pthread_join"
#define LOCK          "pthread_mutex_lock"
#define UNLOCK        "pthread_mutex_unlock"

/* Returns the number of threads created in a module 
*/

int numberOfThreads( llvm::Module* mod ) {
    // TODO: support thread creation in loops
    int cnt = 0;

    llvm::StringRef n( PTHREADCREATE );
    llvm::Function* func = mod->getFunction( n );
    for( auto ui = func->use_begin(); ui != func->use_end(); ui++ ) {
        cnt++;
    }
        
    return cnt;
}

/* Returns the functions called when the threads are created 
 * and the associated thread id
*/

std::map<llvm::Value*, llvm::Function*> functionThreadCreation(  llvm::Module* mod ) {
    std::map<llvm::Value*, llvm::Function*> creation;
    llvm::Value* key;
    llvm::Function* value;

    llvm::StringRef n( PTHREADCREATE );
    llvm::Function* func = mod->getFunction( n );

    /* Get the uses of this function */
    for( auto ui = func->use_begin(); ui != func->use_end(); ui++ ) {
        /* Get the operands, which are actually the arguments passed 
           to the function */
        llvm::User* v = ui->getUser();
        /* arg 0 -> thread name
           arg 2 -> function name */
        key = v->getOperand( 0 );
        /* For arg2 we get a Value which is a pointer to the Function:
           dereference the pointer and get the Function, 
           which is a subclass of Value, therefore we can cast it */
      llvm::Value* tmpval = v->getOperand( 2 );
      llvm::GetElementPtrInst* ptr = static_cast<llvm::GetElementPtrInst*>( tmpval );
      llvm::Value* ptrOp = ptr->getPointerOperand();
      value = static_cast<llvm::Function*>( ptrOp );

       creation[key] = value;
    }
    return creation;
}

/* Get the names of all the global variables of the module
 * Returns a vector that contains these global variables
 */

std::vector<const llvm::GlobalVariable*> getVariables( llvm::Module* mod ) {
    std::vector<const llvm::GlobalVariable*> vec;
    const Module::GlobalListType &globalList = mod->getGlobalList();
    for(auto i = globalList.begin(); i != globalList.end(); i ++) {
        const llvm::GlobalVariable* var = &(*i);
        vec.push_back( var );
    }
    return vec;
}

/* check if all the local variables are allocated in the first 
 * block of the function
 */

bool compliantAllocaFun( llvm::Function* fun ) {

    /* The entry point is always the first block in the vector 
       because blocks are push_back()'ed into this vector */
    
    bool firstblock = true;
    
    /* Get a list of the blocks used by the function */
   
    std::vector<llvm::BasicBlock*> visitedblocks = blocklist( fun ) ;
    for( auto bb = visitedblocks.begin() ; bb != visitedblocks.end() ; bb++ ) {
        if( firstblock ) {
            firstblock = false;
        } else {
            for( auto ins = (*bb)->begin() ; ins != (*bb)->end() ; ins++ ) {
                if( opcodes.AllocaInst == ins->getOpcode() ) {
                    return false;
                }
            }
        }
    }
    return true;
}

/* check if all the functions (main and executed by threads) are compliant
 */

bool compliantAlloca( llvm::Module* mod ) {

    bool rc;

    /* Get the functions we are interested in */
    
    llvm::Function *mainfun = mod->getFunction( llvm::StringRef( "main" ) );
    if( ! (rc = compliantAllocaFun( mainfun ) ) ){
        std::cerr << "Allocation not in the first block in main function" << std::endl;
        return false;
    }

    std::map<llvm::Value*, llvm::Function*> threadsfn = functionThreadCreation( mod );
    for( auto ti = threadsfn.begin() ; ti != threadsfn.end() ; ti++ ) {
        if( ! (rc = compliantAllocaFun( ti->second ) ) ){
            std::cerr << "Allocation not in the first block in function " << ti->second->getName().str() << std::endl;
            return false;
        }
    }

    return true;
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
    GlobalVariable* var = mod->getGlobalVariable( name );
    return (var == NULL ) ? false:true;
}

/* Does this instruction contain a global variable?
 * TODO : pointeurs
 */

void parseInstruction( llvm::Module* mod, llvm::Instruction* ins ) {
    /* look at the operands of the instruction */
    ins->print( errs() );
    std::cerr << "\n";
    std::cerr << "Op code : " << ins->getOpcode() << " (" << ins->getOpcodeName() << ")" << std::endl;

    switch( ins->getOpcode() ) {
        llvm::Value* val;
        llvm::Function *fun;
        
    case opcodes.CallInst:
        /* Look at the arguments passed */
        fun = static_cast<llvm::CallInst*>(ins)->getCalledFunction();
        if(  0 == safeCompareFname( LOCK, fun->getName().str() ) ) {
            val = ins->getOperand( 0 );
            errs() << "MUTEX" << " " << val->getName() ;
            std::cerr << " is TAKEN by " << fun->getName().str() << std::endl;
        }
        if(  0 == safeCompareFname( UNLOCK, fun->getName().str() ) ) {
            val = ins->getOperand( 0 );
            errs() << "MUTEX" << " " << val->getName() ;
            std::cerr << " is RELEASED by " << fun->getName().str() << std::endl;
        }
        break;
    case opcodes.StoreInst:
            /* Store instruction: what do we store, and where? 
             * Syntax: store <value> <pointer>  */
        val = ins->getOperand( 1 );
        if( isGlobal( mod, val->getName() ) ) {
            errs() << val << " " << val->getName() ;
            std::cerr << " is global" << std::endl;
            std::cerr << "WRITE LOCAL VALUE INTO GLOBAL VAR" << std::endl;
        }
        val = ins->getOperand( 0 );
        if( isGlobal( mod, val->getName() ) ) {
            errs() << val << " " << val->getName() ;
            std::cerr << " is global" << std::endl;
            std::cerr << "READ VALUE FROM GLOBAL VAR AND WRITE INTO LOCAL VAR" << std::endl;
        }
        break;
    case opcodes.LoadInst:
        /* Load instruction: what do we load, from where?
           Syntax: <result> = load <pointer> */
        val = ins->getOperand( 0 );
        if( isGlobal( mod, val->getName() ) ) {
            errs() << val << " " << val->getName() ;
            std::cerr << " is global" << std::endl;
        }
        break;

        
        
    default:
        std::cerr << "Op code " << ins->getOpcode() << " not supported yet" << std::endl;
        break;
        
    }



    
    std::cerr << " ---- " << std::endl;
}

/* Get a vector of functions called in a block */

std::vector<llvm::Function*> listOfFunctionsBlock( llvm::BasicBlock* bb ) {

    std::vector<llvm::Function*> functions;
    for( auto ins = bb->begin() ; ins != bb->end() ; ins++ ) {
        if( opcodes.CallInst == ins->getOpcode() ) {
            llvm::Instruction* i = &(*ins);
            llvm::Function* fun = static_cast<llvm::CallInst*>(i)->getCalledFunction();
            if( !itemexists( functions, fun ) ) {
                functions.push_back( fun );
            }

        }
    }
    return functions;
}

/* Get a vector that contains the functions called in a function
 */

std::vector<llvm::Function*> functionsCalledByFunction( llvm::Function* fun ) {

    std::vector<llvm::Function*> funlist;

    /* Get a list of the blocks used by the function */
   
    std::vector<llvm::BasicBlock*> visitedblocks = blocklist( fun ) ;

    /* Get a list of the functions called in each block */

    for( auto block = visitedblocks.begin() ; block != visitedblocks.end() ; block++ ) {
        std::vector<llvm::Function*> thelist = listOfFunctionsBlock( *block );
        appendVector( funlist, thelist );
    }

    return funlist;
}


/* Parse a block and its successors (recursive)
 * TODO : loops
*/

void parseBasicBlock(llvm::Module* mod, llvm::BasicBlock* block ) {
    int line = 0;
    /* Okay, what is inside this block? */
    for( auto i = block->begin() ; i != block->end() ; i++ ) {
        std::cout << "Line " << line << std::endl;
        parseInstruction( mod, i );
        line++;
    }

    TerminatorInst *term = block->getTerminator();
    int numsucc = term->getNumSuccessors();
    std::cerr << "Terminator:" << std::endl;
    std::cerr << "\tI have " << numsucc << " successors" << std::endl;
    for( int i = 0 ; i < numsucc ; i++ ) { // wtf no list here?!?!
        std::cerr << "\tSuccessor " << i << std::endl;
        parseBasicBlock( mod, term-> getSuccessor( i ) );
    }
}

/* Parse the blocks used by a function, starting from its entry point
*/

void parseFunction( llvm::Module* mod, llvm::Function* func ) {
    llvm::BasicBlock* entryBlock = func->begin();
    std::cerr << "Entry block is " << std::endl;
    parseBasicBlock( mod, entryBlock );
}

/* Transform the LLVM IR code into our internal representation
 */

void IRpass( llvm::Module* mod ) {

    /* Analyze every function individually, until pthread_join is called */

    llvm::Function *mainfunc = mod->getFunction( llvm::StringRef( "main" ) );
    std::cerr << "MAIN FUNCTION" << std::endl;        
    parseFunction( mod, mainfunc );

    std::map<llvm::Value*, llvm::Function*> threadCreations = functionThreadCreation( mod );
    for( auto ti = threadCreations.begin() ; ti != threadCreations.end() ; ti++ ) {
        llvm::Value* tid = ti->first;
        llvm::Function* function = ti->second;
        std::cerr << "FUNCTION " << function->getName().str() << std::endl;
        parseFunction( mod, function );
        std::cerr << "------" << std::endl;
    }
}

/* Get the symbols used by a function */

std::vector<std::string>  getSymbolsFun( llvm::Function* fun ) {

    std::vector<std::string>  symbols;
    
    /* Get the first blocks used */
    /* Only the first one, because we assert that all
       the allocations are made in the first block */
    
    //    llvm::BasicBlock* entryBlock = fun->begin();
     
    std::vector<llvm::BasicBlock*> visitedblocks = blocklist( fun ) ;
    
    for( auto bb = visitedblocks.begin() ; bb != visitedblocks.end() ; bb++ ) {
        
        /* Okay, what is inside each block? */
        
        for( auto i = (*bb)->begin() ; i != (*bb)->end() ; i++ ) {
            std::pair<unsigned, unsigned> allo;
            std::string name;
            int nb;name = (i->hasName()) ? i->getName().str() : getShortValueName( i );
            switch( i->getOpcode() ) { /*  TODO: needs to be completed wit other operations */
            case opcodes.LoadInst:
            case opcodes.CallInst:
            case opcodes.ICmpInst:
                break;
            case opcodes.AllocaInst: // case when no name in this case: %1 %2 %3 allocated for argc and argv
                /* Here we need to allocate to memory blocks:
                   one for the allocated memory itself, one for the pointer */
                allo = getAllocaInfo( &(*i) );
                nb = std::ceil( allo.first / 32 );
                //           std::cerr << " allocated size: " << allo.second << " elements of size " << allo.first << " in " << nb << " blocks each" << std::endl;
                /* pointer */
                symbols.push_back( name );
                /* allocated mem */
                for( int i = 0 ; i < (nb*allo.second) ; i++ ) {
                    if( 0 == i )
                        symbols.push_back( "*" + name );
                    else
                        symbols.push_back( "" );
                }
               break;
          default:
                name = "";
                break;
            }
            if( name != "" ) {
                /* Is this variable live? */
                if( isLive( name, fun ) ) {
                    symbols.push_back( name );
                } /*else {
                    std::cerr << name << " is dead" << std::endl;
                    }*/
            }
        }
    }
    return symbols;
}


/* Get the symbols used by all the functions */

std::vector<std::pair<std::string, llvm::Value*>>  getSymbols( llvm::Module* mod ) {
    
    std::vector<std::pair<std::string, llvm::Value*>> symb;

    llvm::Function *mainfun = mod->getFunction( llvm::StringRef( "main" ) );
    std::vector<std::string> s = getSymbolsFun( mainfun );
    for( auto si = s.begin() ; si != s.end() ; si++ ) {
        std::pair<std::string, llvm::Value*> p( *si, NULL );
        symb.push_back( p );
    }
    std::cerr << symb.size() << " symbols in the main function" << std::endl;
    
    std::map<llvm::Value*, llvm::Function*> threadsfn = functionThreadCreation( mod );
    for( auto ti = threadsfn.begin() ; ti != threadsfn.end() ; ti++ ) {
        std::vector<std::string> s = getSymbolsFun( ti->second );
        int cnt = 0;
        for( auto si = s.begin() ; si != s.end() ; si++ ) {
            std::pair<std::string, llvm::Value*> p( *si, ti->first );
            symb.push_back( p );
            cnt++;            
        }
        std::cerr << cnt << " symbols in function " << ti->second->getName().str() <<std::endl;
    }

    return symb;
}

/* Map the symbols of the local variables and the global ones
   with their local address in out machine
*/

machine_t  mapSymbols( std::vector<std::pair<std::string, llvm::Value*>> locVar, std::vector<const llvm::GlobalVariable*> globVar, unsigned nbthreads ){

    machine_t machine;

    /* Save some space for the program counters of the threads
       and the main function */
    
    unsigned idx = nbthreads+1; 

    /* global variables */

    for( auto v = globVar.begin() ; v != globVar.end() ; v++ ) {
        std::pair<std::string, llvm::Value*> p( (*v)->getName().str(), NULL );
        machine[p] = idx;
        std::cerr << "Global variable: " << (*v)->getName().str() << " at idx " << idx << std::endl;
        idx++;
   }

    /* Local variables of each thread */

    for( auto v = locVar.begin() ; v != locVar.end() ; v++ ) {
        machine[*v] = idx;        
        std::cerr << "Local variable: " << (*v).first;
        if( NULL != (*v).second ) {
            std::cerr << " on thread " << (*v).second->getName().str() << " at idx " << idx  << std::endl;;
        } else {
            std::cerr << " on the main thread at idx " << idx  << std::endl;
        }
        idx++;
    }

    return machine;
}

/* Map that contains the inverse of the map returned by mapSymbols
 * For a given unsigned int (position in the machine), return the name of the symbol
 */

machine_inverse_t  mapSymbolsInverse( machine_t machine ){
    std::map<unsigned, std::pair<std::string, llvm::Value*>> inverse;
    for( auto i = machine.begin() ; i != machine.end() ; i++ ) {
        if( i->first.first != "" ){
            inverse[i->second] = i->first;
        }
    }
    return inverse;
}

/* Does the function contain unsupported function calls?
 */

bool unsupportedCalls( llvm::Module* mod, std::string fname, std::vector<std::string> allowedfn ) {
    llvm::StringRef n( fname );
    llvm::Function *fun = mod->getFunction( n );

    std::vector<llvm::Function*> funcs = functionsCalledByFunction( fun );
    std::vector<llvm::Function*> extra = notInList( funcs, allowedfn );

    if( not extra.empty() ) {
        std::cerr << "Function calls not allowed in the " << fname << " function:" << std::endl;
        for( auto f = extra.begin() ; f != extra.end() ; f++ ) {
            std::cerr << (*f)->getName().str() << std::endl;
        }
        return false;
    }
    return true;
}

/* Accept only calls to printf and mutex-related functions 
   in the functions executed by the threads, plus thread 
   creation/destruction in the main function. 
   No other function calls are allowed. */

bool compliantFunCalls( llvm::Module* mod ) {
    bool rc;
    
    /* Visit the functions */

    std::string mainname = "main";
    std::vector<std::string> allowedmain = {"llvm.dbg.declare", "pthread_create", "pthread_join", "printf", "pthread_mutex_lock", "pthread_mutex_unlock", "pthread_mutex_init" };
    
    rc = unsupportedCalls( mod, mainname, allowedmain );
    if( !rc ) {
        return false;
    }
    
    /* functions called when threads are created? */
    
    std::map<llvm::Value*, llvm::Function*> threadsfn = functionThreadCreation( mod );
    std::vector<std::string> allowedthreads = { "printf", "pthread_mutex_lock", "pthread_mutex_unlock", "pthread_mutex_init" };

    for( auto ti = threadsfn.begin() ; ti != threadsfn.end() ; ti++ ) {
        std::string fname = ti->second->getName().str();
        rc = unsupportedCalls( mod, fname, allowedthreads );
        if( !rc ) {
            return false;
        }
    }
    return true;
}

/* Sanity check: 
 * make sure the input file complies with our limitations 
 * TODO : stuff with pointers
 * TODO : thread creation in loops
 */

bool checkcompliance( llvm::Module* mod ){
    bool rc;
    
    if( !(rc = compliantFunCalls( mod ))) {
        return false;
    }

    if( !(rc = compliantAlloca( mod ))){
        return false;
    }

    return true;
}


int readIR( llvm::Module* mod ) {
    bool rc;

    /* Check the file complies with our limitations */
    
    rc = checkcompliance( mod );

    if( rc ) {
        std::cout << "The file is compliant. Continue \\o/" << std::endl;
    } else {
        std::cerr << "The file does not comply with our limitations. Terminating." << std::endl;
        return -1;    
    }
    
    /* Get the names of the global variables of the module */
    
    auto globVar = getVariables( mod );
    
    /* Count the number of threads */
    
    int nbThreads =  numberOfThreads( mod );
    std::cout << nbThreads << " thread creations" << std::endl;

    /* Which are the functions called when threads are created? */
    
    std::map<llvm::Value*, llvm::Function*> threadCreations = functionThreadCreation( mod );

    /* Get the local variables used by each thread */
    
    std::vector<std::pair<std::string, llvm::Value*>> symbols = getSymbols( mod );

    /* the map contains:
     * - a pair: symbol, thread id
     * - its address in our machine
     */
    
    machine_t machinememory = mapSymbols( symbols, globVar, nbThreads );

    /* Reverse map, for debugging purpose */
    
    machine_inverse_t inversemap =  mapSymbolsInverse( machinememory );

    /* Dump the state of the machine (debugging) */

    //  dumpMachine( machinememory );
    
    /* Last phase: transform the code */

    IRpass( mod );

    return 0;
}


int main(int argc, char** argv) {
    int rc;

   
    if (argc < 2) {
        std::cerr << "Expected arguments" << std::endl;
        std::cerr << "\t" << argv[0] << "  <IR file>" << std::endl;
        return EXIT_FAILURE;
    }

    llvm::LLVMContext &context = llvm::getGlobalContext();
    llvm::SMDiagnostic err;
    llvm::Module *mod = llvm::ParseIRFile(argv[1], err, context);

    if (!mod) {
        err.print( argv[0], errs() );
        return 1;
    }

    rc = readIR( mod );
    
    std::cout << "Exiting happily" << std::endl;
    
    return EXIT_SUCCESS;
}
