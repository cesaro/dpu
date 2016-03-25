#include <iostream>
#include "llvm/IR/Constants.h"

#include "sa_utils.h"

using namespace std;
using namespace llvm;

void outputAlloca( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
    
    /* get the name of the pointer that will store the address */
    
    std::string name = (ins->hasName()) ? ins->getName().str() : getShortValueName( ins );

    /* Get the address at which we allocated something in the machine */
    symbol_t symb_p( "*"+ name, tid );
    unsigned addr_p = (*machine)[symb_p];
    symbol_t symb( name, tid );
    unsigned addr = (*machine)[symb];
    
    std::cout << "MOVE i32 [" << addr << "] " << addr_p;
    std::cout << std::endl;
}

void outputLoad( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
    llvm::Value* val;
    std::string name = (ins->hasName()) ? ins->getName().str() : getShortValueName( ins );
    
    /* Load instruction: what do we load, from where?
       Syntax: <result> = load <pointer> */
    
    ins->dump();
    llvm::Value* dst = ins->getOperand( 0 );
    std::string dstname = dst->getName().str();
    symbol_t symb( dstname, tid );

    llvm::Type* ty = ins->getType();
   
    std::cout << "IMOVE " << typeToStr( ty ) << " [" << dst->getName().str() << "] [" << name << "]";
    std::cout << std::endl;
}

void outputStore( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {

    /* Store instruction: what do we store, and where? 
     * Syntax: store <value> <pointer>  */
    
    llvm::Value* val = ins->getOperand( 0 );
    std::string valname = (val->hasName()) ? val->getName().str() : getShortValueName( val );
    symbol_t symb_val( valname, tid );
    
    llvm::Type* ty = val->getType();
    
    llvm::Value* ptr = ins->getOperand( 1 );
    std::string ptrname = (ptr->hasName()) ? ptr->getName().str() : getShortValueName( ptr );
    symbol_t symb_ptr( ptrname, tid );

    std::cout << "STORE " << typeToStr( ty ) << " [" << ptrname << "] " <<  valname;
    std::cout << std::endl;
}

void outputCall( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
        llvm::Function *fun;
        llvm::Value* val;
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

}

void outputReturn( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
    llvm::ReturnInst* ret = static_cast<llvm::ReturnInst*>(ins);
    llvm::Value* val = ret->getReturnValue();
    llvm::Type* ty = val->getType();
    std::string retname = (val->hasName()) ? val->getName().str() : getShortValueName( val );
    std::cout << "RET ";
    if( !isa<llvm::ConstantPointerNull>(val )) {
        std::cout << typeToStr( ty ) << " " << retname ;
    }
    std::cout << std::endl;
}

void outputBranch( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
    llvm::BranchInst* br = static_cast<llvm::BranchInst*>(ins);
    if( br->isUnconditional() ) {
        std::cout << "BR " << br->getSuccessor( 0 ) << std::endl;
    } else {
        llvm::Value* cond = br->getCondition();
        std::string condname = (cond->hasName()) ? cond->getName().str() : getShortValueName( cond );
        std::cout << "BR " << condname << " " << br->getSuccessor( 0 ) << " " << br->getSuccessor( 1 ) << std::endl;

    }
}

void outputUnknown( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
    std::cout << "Unknown operation ";
    ins->print( outs() );
    std::cout << std::endl;
}
