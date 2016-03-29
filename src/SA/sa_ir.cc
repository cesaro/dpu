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

/* In our IR, Muteces are represented by integers.
 * Here we are taking the address of the said mutex in the memory
 * of our machine.
 */
void lockMutex(  machine_t* machine, llvm::Value* mut ) {
    std::cout << "LOCK " << typeToStr( mut->getType() ) << " ";
    symbol_t mutsym( mut->getName().str(), NULL );
    std::cout << "[" << (*machine)[ mutsym ] << "]";
}

void unlockMutex(  machine_t* machine, llvm::Value* mut ) {
    std::cout << "UNLOCK " << typeToStr( mut->getType() ) << " ";
    symbol_t mutsym( mut->getName().str(), NULL );
    std::cout << "[" << (*machine)[ mutsym ] << "]";
}

void startThread( machine_t* machine, llvm::Value* tid ) {
    symbol_t p( "thread", tid );
    std::cout << "UNLOCK T [" << (*machine)[ p ] << "]";
}

void joinThread( machine_t* machine, llvm::Value* tid ) {
    /* the tid is not passed directly but through a register
       in which the tid is LOADed */
    std::string varname = ((tid->hasName()) ? tid->getName().str() : getShortValueName( tid ));
    symbol_t tidsym( varname, NULL );
    std::cout << "LOCK ";
    std::cout << "[" << (*machine)[ tidsym ] << "]";
}

void outputCall( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
    llvm::Function *fun;
    llvm::Value* val;
    
    fun = static_cast<llvm::CallInst*>(ins)->getCalledFunction();
    
    std::string ret = ((ins->hasName()) ? ins->getName().str() : getShortValueName( ins ));
    std::string type = typeToStr(fun->getFunctionType()->getReturnType() );
      /* Which function is called? */
  
   if(  0 == safeCompareFname( LOCK, fun->getName().str() ) ) {
        val = ins->getOperand( 0 );
        lockMutex( machine, val );
    }
    if(  0 == safeCompareFname( UNLOCK, fun->getName().str() ) ) {
        val = ins->getOperand( 0 );
        unlockMutex( machine, val );
    }
    if(  0 == safeCompareFname( PRINTF, fun->getName().str() ) ) {
        val = ins->getOperand( 0 );
        std::string toprint = (val->hasName()) ? val->getName().str() : getShortValueName( val );
        std::cout << "PRINTF " << type << " " << ret << " " << toprint;
    }
    if(  0 == safeCompareFname( PTHREADCREATE, fun->getName().str() ) ) {
        /* Thread creation. Unlock the thread */
        val = ins->getOperand( 0 );
        startThread( machine, val );
    }
    if(  0 == safeCompareFname( PTHREADJOIN, fun->getName().str() ) ) {
        /* Thread creation. Unlock the thread */
        val = ins->getOperand( 0 );
        joinThread( machine, val );
    }

    std::cout << std::endl;
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

void outputIcmp( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
    llvm::ICmpInst* cmp = static_cast<llvm::ICmpInst*>(ins);
    llvm::CmpInst::Predicate pr = cmp->getPredicate();
    std::cout << "CMP ";
    switch( pr ) {
    case llvm::CmpInst::ICMP_EQ:
        std::cout << "eq ";
        break;
    case llvm::CmpInst::ICMP_NE:
        std::cout << "ne ";
        break;
    case llvm::CmpInst::ICMP_UGT:
        std::cout << "ugt ";
        break;
    case llvm::CmpInst::ICMP_UGE:
        std::cout << "uge ";
        break;
    case llvm::CmpInst::ICMP_SGT:
        std::cout << "sgt ";
        break;
    case llvm::CmpInst::ICMP_SGE:
        std::cout << "sge ";
        break;
    case llvm::CmpInst::ICMP_SLT:
        std::cout << "slt ";
        break;
    case llvm::CmpInst::ICMP_SLE:
        std::cout << "sle ";
       break;
    default:
        std::cerr << "OTHER PREDICATE" << std::endl;
    }

    std::string dest = (ins->hasName()) ? ins->getName().str() : getShortValueName( ins );
    llvm::Value* src1 = ins->getOperand( 0 );
    llvm::Value* src2 = ins->getOperand( 1 );
    std::string src1name = (src1->hasName()) ? src1->getName().str() : getShortValueName( src1 );
    std::string src2name = (src2->hasName()) ? src2->getName().str() : getShortValueName( src2 );
    
    std::cout << dest << " " << src1name << " " << src2name << std::endl;
}

void outputUnknown( llvm::Module* mod, llvm::Instruction* ins, machine_t* machine, llvm::Value* tid ) {
    std::cout << "Unknown operation ";
    ins->print( outs() );
    std::cout << std::endl;
}
