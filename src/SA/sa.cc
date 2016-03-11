#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/CFG.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "sa_utils.h"

using namespace std;
using namespace llvm;

#define PTHREADCREATE "pthread_create"

int numberOfThreads( llvm::Module* mod ) {
    // TODO: support thread creation in loops
    int cnt = 0;
    for( auto fi = mod->getFunctionList().begin() ;
         fi != mod->getFunctionList().end() ;
         fi++ ) {
        if( 0 == safeCompareFname(  PTHREADCREATE, fi->getName().str() ) ){
            cnt++;
        }
    }
    return cnt;
}

void getVariables( llvm::Module* mod ) {
    std::cout << " == Global variables == " << std::endl;
    const Module::GlobalListType &globalList = mod->getGlobalList();
    for(auto i = globalList.begin(); i != globalList.end(); i ++) {
        std::cout << i->getName().str() << std::endl;
    }
}

int readIR( llvm::Module* mod ) {


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
   }

    
    std::cout << " === " << std::endl;

    std::cout << numberOfThreads( mod ) << " thread creations" << std::endl;
    
    std::cout << " === " << std::endl;
    getVariables( mod );

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
