#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/CFG.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace llvm;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Expected arguments" << std::endl;
        std::cerr << "\t" << argv[0] << "  <IR file>" << std::endl;
        return EXIT_FAILURE;
    }

    llvm::LLVMContext &Context = llvm::getGlobalContext();
    llvm::SMDiagnostic Err;
    llvm::Module *Mod = llvm::ParseIRFile(argv[1], Err, Context);

    if (!Mod) {
        Err.print( argv[0], errs() );
        return 1;
    }

    std::cout << "Exiting happily" << std::endl;
    
    return EXIT_SUCCESS;
}
