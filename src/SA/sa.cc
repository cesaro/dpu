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

/* Is this variable a global variable? 
 */

bool isGlobal( llvm::Module* mod, const char* name ){
    llvm::StringRef n( name );
    GlobalVariable* var = mod->getGlobalVariable( n );
    return (var == NULL ) ? false:true;
}

/* Parse a block and its successors (recursive)
*/

void parseBasicBlock( llvm::BasicBlock* block ) {
    /* Okay, what is inside this block? */
    for( auto i = block->begin() ; i != block->end() ; i++ ) {
        //std::cout << "Instruction: " << i->str() << std::endl;
        i->print( errs() );
        std::cerr << std::endl;
    }

    TerminatorInst *term = block->getTerminator();
    int numsucc = term->getNumSuccessors();
    std::cerr << "Terminator:" << std::endl;
    std::cerr << "\tI have " << numsucc << " successors" << std::endl;
    for( int i = 0 ; i < numsucc ; i++ ) { // wtf no list here?!?!
        std::cerr << "\tSuccessor " << i << std::endl;
        parseBasicBlock( term-> getSuccessor( i ) );
    }
}

/* Parse the blocks used by a function, starting from its entry point
*/

void parseFunction( llvm::Function* func ) {
    llvm::BasicBlock* entryBlock = func->begin();
    std::cerr << "Entry block is " << std::endl;
    parseBasicBlock( entryBlock );
}

int readIR( llvm::Module* mod ) {

    /* Get the names of the global variables of the module */
    auto globVar = getVariables( mod );

    /* Count the number of threads */
    int nbThreads =  numberOfThreads( mod );
    std::cout << nbThreads << " thread creations" << std::endl;

    /* Which are the functions called when threads are created? */
    std::map<llvm::Value*, llvm::Function*> threadCreations = functionThreadCreation( mod );

    /* Analyze every function individually, until pthread_join is called */
    for( auto ti = threadCreations.begin() ; ti != threadCreations.end() ; ti++ ) {
        llvm::Value* tid = ti->first;
        llvm::Function* function = ti->second;
        /* parse the blocks used by this function and get the trace */
        parseFunction( function );
        std::cerr << "------" << std::endl;
    }



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
