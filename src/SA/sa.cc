#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/CFG.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

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

#include "llvm/IR/CallSite.h"


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

std::map<llvm::Argument*, llvm::Argument*> functionThreadCreation(  llvm::Module* mod ) {
    std::map<llvm::Argument*, llvm::Argument*> creation;
    llvm::Argument* key;
    llvm::Argument* value;

    for( auto fi = mod->getFunctionList().begin() ;
         fi != mod->getFunctionList().end() ;
         fi++ ) {
        if( 0 == safeCompareFname(  PTHREADCREATE, fi->getName().str() ) ){
            /* Okay, this is a thread creation -- parse the arguments */
            for( auto arg = fi->getArgumentList().begin();
                 arg != fi->getArgumentList().end();
                 arg++ ) {
                /* arg 0 -> thread name
                   arg 2 -> function name */
                if( 0 == arg->getArgNo() ) {
                    key = arg;
                }
                if( 2 == arg->getArgNo() ) {
                    value = arg;
                }
            } /* end for arg */
            creation[key] = value;
        } /* end if */
    } /* end for fi */
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

void parseBasicBlock( llvm::BasicBlock* block ) {
    /* Okay, what is inside this block? */
    for( auto i = block->begin() ; i != block->end() ; i++ ) {
        //std::cout << "Instruction: " << i->str() << std::endl;
        i->print( errs() );
        std::cerr << std::endl;
    }
}

void parseFunction( llvm::Function* func ) {
    /* Iterate over the blocks of a function */
    for( auto i = func->begin() ; i != func->end(); i++ ) {
   errs() << "Basic block (name=" << i->getName() << ") has "
             << i->size() << " instructions.\n";
       parseBasicBlock( i );
    }
}

int readIR( llvm::Module* mod ) {

    /* Get the names of the global variables of the module */
    auto globVar = getVariables( mod );

    /* Count the number of threads */
    int nbThreads =  numberOfThreads( mod );
    std::cout << nbThreads << " thread creations" << std::endl;

    /* Which are the functions called when threads are created? */
    std::map<llvm::Argument*, llvm::Argument*> threadCreations = functionThreadCreation( mod );

    /* Analyze every function individually, until pthread_join is called */
    for( auto ti = threadCreations.begin() ; ti != threadCreations.end() ; ti++ ) {
        ti->first->dump();
        ti->second->dump();
        std::cout << ti->first->getName().str() << "Calling " << ti->second->getName().str() << std::endl;
        
    }

    
    return 0;

    //    llvm::Module::FunctionListType functions = mod->getFunctionList ();
    for( llvm::Module::iterator fi = mod->getFunctionList().begin() ;
         fi != mod->getFunctionList().end() ;
         fi++ ) {
        std::cout << fi->getName().str() << std::endl;
    }

    std::cout << " === " << std::endl;
    
    llvm::Function *EntryFn = mod->getFunction( "main" );

    
    for (Module::iterator I = mod->begin() ; I != mod->end(); I++) {
       Function *Fn = &*I;
       if( Fn == EntryFn ) 
           std::cout << "entry fn" << std::endl;
       parseFunction( I );
   }

    
    std::cout << " === " << std::endl;

    std::cout << numberOfThreads( mod ) << " thread creations" << std::endl;
    
    std::cout << " === " << std::endl;

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



/*     for( auto fi = mod->getFunctionList().begin() ;
         fi != mod->getFunctionList().end() ;
         fi++ ) {
        std::cout << "Function " << fi->getName().str() << std::endl;
        llvm::AttributeSet attr = fi->getAttributes ();
        //    attr.dump();
        auto acc = mod -> getSublistAccess( fi );
        
        //        for(  ValueMap::iterator
        std::cout << "---" << std::endl;
}
*/
