
#include "fe/llvm/util.hh"
#include "fe/llvm/util.tpp"
#include "fe/llvm/sa_ir.h"
#include "fe/llvm/parser.hh"

#include "fe/ir.hh"
#include "misc.hh"

/** 
 * - get the names of all the global variables
 * - get the name of the function called when threads are created
 * - start analyzing the code at the beginning of this function
 **/

namespace dpu {
namespace fe {
namespace llvm {

using namespace std;

bool compliantFunCalls( llvm::Module* mod );
bool compliantAlloca( llvm::Module* mod );
std::vector<std::pair<std::string, llvm::Value*>>  getSymbolsFun( llvm::Function* fun );

class Parser
{
public:
	Parser (llvm::Module & m) : mod (&m), p (0), errors (false), nbThreads (0) {}
	~Parser ();

	void parse ();

	bool got_errors () { return errors; }
	std::string get_errmsg () { return errmsg; }
	ir::Program * get_program () { ir::Program * pp = p; p = 0; return pp; }

private:
	llvm::Module * mod;
	ir::Program * p;
	bool errors;
	std::string errmsg;

	std::vector<const llvm::GlobalVariable*> globalVariables;
	std::map<llvm::Value*, llvm::Function*> threadCreations;
	std::map<llvm::Value*, ir::Function*>   pid2irfun;
	std::map<llvm::Value*, ir::Symbol*>     pid2lock;
    std::map<symbol_t, llvm::Value*>        localSymbols;
    int nbThreads;

	int checkcompliance ();
	void getGlobalVariables ();
    void numberOfThreads ();
	void functionThreadCreation ();
	void getLocalSymbols ();
    void mapSymbols ();

	unsigned val2datasize (const llvm::Value * v);
};

Parser::~Parser ()
{
	delete p;
}

void Parser::parse ()
{
	int ret;

	errors = false;
    ret = checkcompliance ();
    if (ret != 0) {
        errors = true;
		errmsg = "The file does not comply with our limitations";
        return;
    }

    /* Count the number of threads */
    numberOfThreads ();
	DEBUG2 ("%d thread creations", nbThreads + 1);

    /* create the program container */
    p = new ir::Program (nbThreads + 1);

    /* Which are the functions called when threads are created? */
    /* the map contains:
     * - a pair: symbol, thread id
     * - its address in our machine
     */
    functionThreadCreation ();
    for (auto & pp : threadCreations)
    {
    	std::string s = fmt ("%p_%s", pp.first, pp.second->getName().str().c_str());
		ir::Function * f = p->add_thread (s);
		pid2irfun[pp.first] = f;
    }
	pid2irfun[nullptr] = p->add_thread ("0x0_main");

    /* Get the names of the global variables of the module and local registers per function */
	DEBUG2 ("here");
    getGlobalVariables ();
	DEBUG2 ("here");
    getLocalSymbols ();

#if 0
	for (auto & it = globalVariables.begin(); it != globalVariables.end(); ++it)
	{
		DEBUG
	}
#endif

	DEBUG2 ("here");

	/* allocate memory space in the program container for all LLVM symbols */
    mapSymbols ();
#if 0

    /* Reverse map, for debugging purpose */
    
    machine_inverse_t inversemap =  mapSymbolsInverse( machinememory );

    /* Last phase: transform the code */

    IRpass( mod, &machinememory );

    /* Dump the state of the machine (debugging) */

    // dumpMachine( &machinememory );
    
    return 0;
#endif
}


int Parser::checkcompliance ()
{
    bool rc;
    
    if( !(rc = compliantFunCalls (mod))) {
        return -1;
    }

    if( !(rc = compliantAlloca (mod))){
        return -1;
    }

    return 0;
}

void Parser::getGlobalVariables ()
{
    globalVariables.clear ();
    const llvm::Module::GlobalListType &globalList = mod->getGlobalList();
    for(auto i = globalList.begin(); i != globalList.end(); i ++) {
        const llvm::GlobalVariable* var = &(*i);
        globalVariables.push_back( var );
    }
}

void Parser::numberOfThreads ()
{
    // TODO: support thread creation in loops
    int cnt = 0;

    llvm::StringRef n( "pthread_create" );
    llvm::Function* func = mod->getFunction( n );
    for( auto ui = func->use_begin(); ui != func->use_end(); ui++ ) {
        cnt++;
    }
        
    nbThreads = cnt;
}

void Parser::functionThreadCreation ()
{
    llvm::Value* key;
    llvm::Function* value;

    llvm::StringRef n( "pthread_create" );
    llvm::Function* func = mod->getFunction( n );

	threadCreations.clear ();

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

       threadCreations[key] = value;
    }
}

void Parser::getLocalSymbols ()
{
	localSymbols.clear ();

    llvm::Function *mainfun = mod->getFunction( llvm::StringRef( "main" ) );
    auto s = getSymbolsFun( mainfun );
    for( auto si = s.begin() ; si != s.end() ; si++ ) {
		localSymbols[ symbol_t (si->first, nullptr)] = si->second;
    }
    std::cerr << localSymbols.size() << " symbols in the main function" << std::endl;
    
    for( auto ti = threadCreations.begin() ; ti != threadCreations.end() ; ti++ ) {
        auto s = getSymbolsFun( ti->second );
        int cnt = 0;
        for( auto si = s.begin() ; si != s.end() ; si++ ) {
			localSymbols[ symbol_t (si->first, ti->first)] = si->second;
            cnt++;            
        }
        std::cerr << cnt << " symbols in function " << ti->second->getName().str() <<std::endl;
    }
}

unsigned Parser::val2datasize (const llvm::Value * v)
{
	unsigned s = v->getType()->getPrimitiveSizeInBits ();
	return s == 0 ? 4 : s / 8;
}

void Parser::mapSymbols ()
{
    /* Save some space for the program counters of the threads
       and the main function */
    
    /* thread creation/destruction is a SYNC on specific lock */
    for( auto ti = threadCreations.begin() ; ti != threadCreations.end() ; ti++ ) {
        std::string s = fmt ("__thr_lock_%s_%p", ti->second->getName().str().c_str(), ti->first);
        ir::Symbol * sym = p->module.allocate (s.c_str(), 4, 4, 1);
        pid2lock[ti->first] = sym;
    }

    /* global variables */
    for( auto v = globalVariables.begin() ; v != globalVariables.end() ; v++ ) {
		SHOW (*v, "p");
        std::string s = fmt ("glo_%s", (*v)->getName().str().c_str());
		unsigned ds = val2datasize (*v);
		// FIXME -- initial value for global symbols !!!
		#if 0
		std::string st = v->getName().str();
		if( st == "" ) {
		    st = getShortValueName(*v);
			        p->module.allocate (s.c_str(), ds, ds); , std::stoi( getShortValueName(*v)) );

		}
        SHOW (getShortValueName(*v).c_str(), "s");
		#endif
		SHOW (s.c_str(), "s");
        p->module.allocate (s.c_str(), ds, ds);
		DEBUG2 ("allocate: global '%s' init '%lld'", 
				(*v)->getName().str().c_str(), 0);
    }

    /* Local variables of each thread */
    for(auto pp = localSymbols.begin() ; pp != localSymbols.end() ; pp++ ) {
        std::string s = fmt ("%s_%p", pp->first.first.c_str(), pp->first.second);
		unsigned ds = val2datasize (pp->second);
		// FIXME -- initial value for local symbols !!!
        p->module.allocate (s.c_str(), ds, ds);

		DEBUG2 ("allocate: register '%s' pid '%p' fun '%s'", 
				pp->first.first.c_str(), 
				pp->first.second, 
				pid2irfun[pp->second]->name.c_str());
    }
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx



/* Returns the number of threads created in a module 
*/

int numberOfThreads( llvm::Module* mod ) {
    // TODO: support thread creation in loops
    int cnt = 0;

    llvm::StringRef n( "pthread_create" );
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

    llvm::StringRef n( "pthread_create" );
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
    const llvm::Module::GlobalListType &globalList = mod->getGlobalList();
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


/* Does this instruction contain a global variable?
 * TODO : pointeurs
 */

void parseInstruction( llvm::Module* mod, machine_t* machine, llvm::Instruction* ins, llvm::Value* tid ) {
    /* look at the operands of the instruction */
    ins->print( llvm::errs() );
    std::cerr << "\n";
    std::cerr << "Op code " << ins->getOpcode() << " " << ins->getOpcodeName() << std::endl;

    switch( ins->getOpcode() ) {
        //llvm::Value* val;
        //llvm::Function *fun;
        
    case opcodes.CallInst:
        outputCall( mod, ins, machine, tid );
        break;
    case opcodes.StoreInst:
        outputStore( mod, ins, machine, tid );
        break;
    case opcodes.AllocaInst:
        outputAlloca( mod, ins, machine, tid );
        break;
    case opcodes.LoadInst:
        outputLoad( mod, ins, machine, tid );
        break;
    case opcodes.ReturnInst:
        outputReturn( mod, ins, machine, tid );
        break;
    case opcodes.BranchInst:
        outputBranch( mod, ins, machine, tid );
        break;
    case opcodes.ICmpInst:
        outputIcmp( mod, ins, machine, tid );
        break;

        
    default:
        std::cerr << "Op code " << ins->getOpcode() << " not supported yet" << std::endl;
        outputUnknown( mod, ins, machine, tid );
        std::cout << std::endl;
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

void parseBasicBlock(llvm::Module* mod, machine_t* machine, llvm::BasicBlock* block, llvm::Value* tid ) {
    int line = 0;
    /* Okay, what is inside this block? */
    for( auto i = block->begin() ; i != block->end() ; i++ ) {
        printLineNb( line, block, tid );                   
        parseInstruction( mod, machine, i, tid );
        line++;
    }

    llvm::TerminatorInst *term = block->getTerminator();
    int numsucc = term->getNumSuccessors();
    std::cerr << "Terminator:" << std::endl;
    std::cerr << "\tI have " << numsucc << " successors" << std::endl;
    for( int i = 0 ; i < numsucc ; i++ ) { // wtf no list here?!?!
        std::cerr << "\tSuccessor " << i << std::endl;
        parseBasicBlock( mod, machine, term-> getSuccessor( i ), tid );
    }
}

/* Parse the blocks used by a function, starting from its entry point
*/

void parseFunction( llvm::Module* mod, machine_t* machine, llvm::Function* func, llvm::Value* tid ) {
    llvm::BasicBlock* entryBlock = func->begin();
    std::cerr << "Entry block is " << std::endl;
    parseBasicBlock( mod, machine, entryBlock, tid );
}

/* Transform the LLVM IR code into our internal representation
 */

void IRpass( llvm::Module* mod, machine_t* machine ) {

    /* Analyze every function individually, until pthread_join is called */

    llvm::Function *mainfunc = mod->getFunction( llvm::StringRef( "main" ) );
    std::cerr << "MAIN FUNCTION" << std::endl;        
    parseFunction( mod, machine, mainfunc, NULL );

    std::map<llvm::Value*, llvm::Function*> threadCreations = functionThreadCreation( mod );
    for( auto ti = threadCreations.begin() ; ti != threadCreations.end() ; ti++ ) {
        llvm::Value* tid = ti->first;
        llvm::Function* function = ti->second;
        std::cerr << "FUNCTION " << function->getName().str() << std::endl;
        parseFunction( mod, machine, function, tid );
        std::cerr << "------" << std::endl;
    }
}

/* Get the symbols used by a function */

std::vector<std::pair<std::string, llvm::Value*>>  getSymbolsFun( llvm::Function* fun ) {

	std::vector<std::pair<std::string, llvm::Value*>> symbols;
    
    /* Get the first blocks used */
    /* Only the first one, because we assert that all
       the allocations are made in the first block */
    
    //    llvm::BasicBlock* entryBlock = fun->begin();

	DEBUG ("getSymbolsFun: at '%s'", fun->getName().str().c_str());
     
    std::vector<llvm::BasicBlock*> visitedblocks = blocklist( fun ) ;
    
    for( auto bb = visitedblocks.begin() ; bb != visitedblocks.end() ; bb++ ) {
        
        /* Okay, what is inside each block? */
        
        for( auto i = (*bb)->begin() ; i != (*bb)->end() ; i++ ) {
            std::pair<unsigned, unsigned> allo;
            std::string name;
            int nb;
            name = (i->hasName()) ? i->getName().str() : getShortValueName( i );
            switch( i->getOpcode() ) { /*  TODO: needs to be completed wit other operations */
			case opcodes.StoreInst:
			case opcodes.ReturnInst:
			case opcodes.BranchInst:
				// zero registers to handle
				break;

            case opcodes.ICmpInst:
            case opcodes.CallInst:
            case opcodes.LoadInst:
				// one register to handle
                /* Is this variable live? */
                if( isLive( name, fun ) ) {
                    symbols.emplace_back( name, &(*i) );
                } /*else {
                    std::cerr << name << " is dead" << std::endl;
                    }*/
                break;

            case opcodes.AllocaInst: // case when no name in this case: %1 %2 %3 allocated for argc and argv
                /* Here we need to allocate to memory blocks:
                   one for the allocated memory itself, one for the pointer */
                allo = getAllocaInfo( &(*i) );
                nb = std::ceil( allo.first / 32 );
                //           std::cerr << " allocated size: " << allo.second << " elements of size " << allo.first << " in " << nb << " blocks each" << std::endl;
                /* pointer */
                symbols.emplace_back( name, &(*i) );
				symbols.emplace_back( "*" + name, &(*i) );

               break;
			default:
		  		std::string s;
		  		llvm::raw_string_ostream os (s);
		  		i->print (os);
				os.flush ();
				DEBUG ("Unsupported instruction when extracting local symbols: '%s'", s.c_str ());
		  		ASSERT (0);
                name = "";
                break;
            }
        }
    }
    return symbols;
}


/* Get the symbols used by all the functions */

std::vector<symbol_t>  getLocalSymbols( llvm::Module* mod ) {
    
    std::vector<symbol_t> symb;
#if 0

    llvm::Function *mainfun = mod->getFunction( llvm::StringRef( "main" ) );
    std::vector<std::string> s = getSymbolsFun( mainfun );
    for( auto si = s.begin() ; si != s.end() ; si++ ) {
        symbol_t p( *si, NULL );
        symb.push_back( p );
    }
    std::cerr << symb.size() << " symbols in the main function" << std::endl;
    
    std::map<llvm::Value*, llvm::Function*> threadsfn = functionThreadCreation( mod );
    for( auto ti = threadsfn.begin() ; ti != threadsfn.end() ; ti++ ) {
        std::vector<std::string> s = getSymbolsFun( ti->second );
        int cnt = 0;
        for( auto si = s.begin() ; si != s.end() ; si++ ) {
            symbol_t p( *si, ti->first );
            symb.push_back( p );
            cnt++;            
        }
        std::cerr << cnt << " symbols in function " << ti->second->getName().str() <<std::endl;
    }

#endif
    return symb;
}

/* Map the symbols of the local variables and the global ones
   with their local address in out machine
*/

machine_t  mapSymbols( std::vector<symbol_t> locVar, std::vector<const llvm::GlobalVariable*> globVar, unsigned nbthreads, std::map<llvm::Value*, llvm::Function*> threadfunctions ){

    machine_t machine;

    /* Save some space for the program counters of the threads
       and the main function */
    
    unsigned idx = nbthreads+1;

    /* thread creation/destruction is a SYNC on specific lock */

     for( auto ti = threadfunctions.begin() ; ti != threadfunctions.end() ; ti++ ) {
         symbol_t p( "thread", ti->first );
         machine[p] = idx;
         idx++;
     }

    /* global variables */

    for( auto v = globVar.begin() ; v != globVar.end() ; v++ ) {
        symbol_t p( (*v)->getName().str(), NULL );
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
    machine_inverse_t inverse;
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

std::unique_ptr<ir::Program> translate (llvm::Module* mod )
{
    bool rc;
    //return std::unique_ptr<ir::Program> (new ir::Program (2));

    /* Check the file complies with our limitations */
    
    rc = checkcompliance( mod );

    if( rc ) {
        std::cout << "The file is compliant. Continue \\o/" << std::endl;
    } else {
        std::cerr << "The file does not comply with our limitations. Terminating." << std::endl;
        return 0;
    }
    
    /* Get the names of the global variables of the module */
    
    auto globVar = getVariables( mod );
    
    /* Count the number of threads */
    int nbThreads =  numberOfThreads( mod );
    std::cout << nbThreads << " thread creations" << std::endl;

    /* create the program container */
    std::unique_ptr<ir::Program> p (new ir::Program (nbThreads));

    /* Which are the functions called when threads are created? */
    std::map<llvm::Value*, llvm::Function*> threadCreations = functionThreadCreation( mod );
    for (auto & pp : threadCreations)
    {
    	std::string s = fmt ("%p_%s", pp.first, pp.second->getName().str().c_str());
    }

    /* Get the local variables used by each thread */
    std::vector<symbol_t> symbols = getLocalSymbols( mod );

    /* the map contains:
     * - a pair: symbol, thread id
     * - its address in our machine
     */
    
    machine_t machinememory = mapSymbols( symbols, globVar, nbThreads, threadCreations );

    /* Reverse map, for debugging purpose */
    
    machine_inverse_t inversemap =  mapSymbolsInverse( machinememory );

    /* Last phase: transform the code */

    IRpass( mod, &machinememory );

    /* Dump the state of the machine (debugging) */

    // dumpMachine( &machinememory );
    
    return 0;
}

std::unique_ptr<ir::Program> parse (const std::string & filename, std::string & errors)
{
    // get a context
    llvm::LLVMContext &context = llvm::getGlobalContext();
    llvm::SMDiagnostic err;

    // parse the .ll file and get a Module out of it
#if LLVM_VERSION_MINOR <= 5
    std::unique_ptr<llvm::Module> mod = llvm::ParseIRFile (path, err, context);
#else
    std::unique_ptr<llvm::Module> mod = llvm::parseIRFile (filename, err, context);
#endif

    // if errors found, write them to errors and return
    if (! mod.get ()) {
        llvm::raw_string_ostream os (errors);
        err.print (filename.c_str(), os);
        os.flush ();
        return 0;
    }

	//SHOW (mod.get (), "p");
	//mod.get()->dump ();

	// traslate the LLVM module into our 3 address code
	Parser p (*mod.get ());
	p.parse ();
	if (p.got_errors ())
	{
		DEBUG2 ("here");
		SHOW (p.get_errmsg ().c_str(), "s");
		errors = p.get_errmsg ();
		return 0;
	}
	return std::unique_ptr<ir::Program> (p.get_program ());
}

} } } // namespace dpu::fe::llvm

