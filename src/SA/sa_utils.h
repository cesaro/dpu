#include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/CFG.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/Instructions.h"


using namespace std;
using namespace llvm;

/**********************************************************************/
/*   Constants                                                        */
/**********************************************************************/

#define POINTERSIZE 32
#define PTHREADCREATE "pthread_create"
#define PTHREADJOIN   "pthread_join"
#define LOCK          "pthread_mutex_lock"
#define UNLOCK        "pthread_mutex_unlock"


/**********************************************************************/
/*   Type aliases                                                     */
/**********************************************************************/

using symbol_t = std::pair<std::string, llvm::Value*>;
using machine_t = std::map<symbol_t, unsigned>;
using machine_inverse_t = std::map<unsigned, symbol_t>;

/**********************************************************************/
/*   Prototypes                                                       */
/**********************************************************************/

template <class T> bool itemexists( std::vector<T>, T);
template <class T> void appendVector( std::vector<T>, std::vector<T> );

int safeCompareFname( const char*, std::string);
void dumpBlock( llvm::BasicBlock* );
std::string getShortValueName( llvm::Value *);
bool isLive( std::string, llvm::Function*);
std::vector<llvm::Function*> notInList( std::vector<llvm::Function*>, std::vector<std::string> );
std::vector<llvm::BasicBlock*> blocklist( llvm::Function* );
std::pair<unsigned, unsigned> getAllocaInfo( llvm::Instruction*);
void  dumpMachine(  machine_t* );
bool isGlobal( llvm::Module*, llvm::StringRef );
std::string typeToStr( Type*);
void printLineNb( int, llvm::BasicBlock*, llvm::Value* );

/**********************************************************************/
/*   Classes definition                                               */
/**********************************************************************/


namespace fe { // Front-end
    class Machine {
    public:
        std::map<std::string, int> symtab;
        unsigned                   memsize;

        Machine(){}
    private:
    };
    
    class Trans {
    public:
        enum {RD, WR, LOC, SYN} type;
        

    };

    class Thread {};
    

    
} // end namespace fe

class OpCodes {
 public:

    static const unsigned ReturnInst = llvm::Instruction::Ret;
    static const unsigned BranchInst = llvm::Instruction::Br;
    static const unsigned AllocaInst = llvm::Instruction::Alloca;
    static const unsigned LoadInst = llvm::Instruction::Load;
    static const unsigned StoreInst = llvm::Instruction::Store;
    static const unsigned BitCastInst = llvm::Instruction::BitCast;
    static const unsigned ICmpInst = llvm::Instruction::ICmp;
    static const unsigned CallInst = llvm::Instruction::Call;
    static const unsigned FCmpInst = llvm::Instruction::FCmp;
    static const unsigned AddrSpaceCastInst = llvm::Instruction::AddrSpaceCast;
    static const unsigned PtrToIntInst = llvm::Instruction::PtrToInt;
    static const unsigned IntToPtrInst = llvm::Instruction::IntToPtr;
    static const unsigned FPToSIInst = llvm::Instruction::FPToSI;
    static const unsigned FPToUIInst = llvm::Instruction::FPToUI;
    static const unsigned SIToFPInst = llvm::Instruction::SIToFP;
    static const unsigned UIToFPInst = llvm::Instruction::UIToFP;
    static const unsigned FPExtInst = llvm::Instruction::FPExt;
    static const unsigned FPTruncInst = llvm::Instruction::FPTrunc;
    static const unsigned SExtInst = llvm::Instruction::SExt;
    static const unsigned ZExtInst = llvm::Instruction::ZExt;
    static const unsigned TruncInst = llvm::Instruction::Trunc;
    static const unsigned UnreachableInst = llvm::Instruction::Unreachable;

    static const unsigned ResumeInst = llvm::Instruction::Resume;
    static const unsigned InvokeInst = llvm::Instruction::Invoke;
    static const unsigned IndirectBrInst = llvm::Instruction::IndirectBr;
    static const unsigned SwitchInst = llvm::Instruction::Switch;
    static const unsigned LandingPadInst = llvm::Instruction::LandingPad;
    static const unsigned PHINode = llvm::Instruction::PHI;
    static const unsigned InsertValueInst = llvm::Instruction::InsertValue;
    static const unsigned ExtractValueInst = llvm::Instruction::ExtractValue;
    static const unsigned ShuffleVectorInst = llvm::Instruction::ShuffleVector;
    static const unsigned InsertElementInst = llvm::Instruction::InsertElement;
    static const unsigned ExtractElementInst = llvm::Instruction::ExtractElement;
    static const unsigned VAArgInst = llvm::Instruction::VAArg;
    static const unsigned SelectInst = llvm::Instruction::Select;
    static const unsigned GetElementPtrInst = llvm::Instruction::GetElementPtr;
    static const unsigned AtomicRMWInst = llvm::Instruction::AtomicRMW;
    static const unsigned AtomicCmpXchgInst = llvm::Instruction::AtomicCmpXchg;
    static const unsigned FenceInst = llvm::Instruction::Fence;



#if 0
    static const unsigned CleanupReturnInst = llvm::Instruction::CleanupRet;
    static const unsigned CatchReturnInst = llvm::Instruction::CatchRet;
    static const unsigned CatchPadInst = llvm::Instruction::CatchPad;
    static const unsigned CleanupPadInst = llvm::Instruction::CleanupPad;
    static const unsigned CatchSwitchInst = llvm::Instruction::CatchSwitch;


    static const unsigned InstrProfValueProfileInst = llvm::Instruction::xxxxx;
    static const unsigned InstrProfIncrementInst = llvm::Instruction::  ;
    static const unsigned VACopyInst = llvm::Instruction::  ;
    static const unsigned VAEndInst = llvm::Instruction::  ;
    static const unsigned GCRelocateInst = llvm::Instruction::  ;
    static const unsigned UnaryInstruction = llvm::Instruction:: ;
    static const unsigned VAStartInst = llvm::Instruction::  ;
    static const unsigned MemMoveInst = llvm::Instruction::  ;
    static const unsigned MemCpyInst = llvm::Instruction::  ;
    static const unsigned MemTransferInst = llvm::Instruction::  ;
    static const unsigned MemSetInst = llvm::Instruction::  ;
    static const unsigned MemIntrinsic;
    static const unsigned DbgValueInst = llvm::Instruction::  ;
    static const unsigned DbgDeclareInst = llvm::Instruction::  ;
    static const unsigned TerminatorInst = llvm::Instruction::  ;
    static const unsigned DbgInfoIntrinsic = llvm::Instruction:: ;
    static const unsigned IntrinsicInst = llvm::Instruction::  ;
    static const unsigned CastInst = llvm::Instruction::cccc;
    static const unsigned FuncletPadInst = llvm::Instruction::ccccc;
    static const unsigned BinaryOperator = llvm::Instruction::xxxx;;
#endif

 public:
    OpCodes(){}
};
static OpCodes opcodes;

