template <class T> bool itemexists( std::vector<T>, T);
int safeCompareFname( const char*, std::string);
void dumpBlock( llvm::BasicBlock* );
std::vector<llvm::Function*> notInList( std::vector<llvm::Function*>, std::vector<std::string> );
std::vector<llvm::BasicBlock*> blocklist( llvm::Function* );


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

   static const unsigned ReturnInst = 1;
   static const unsigned BranchInst = 2;
   static const unsigned AllocaInst = 26;
   static const unsigned LoadInst = 27;
   static const unsigned StoreInst = 28;
   static const unsigned BitCastInst = 44;
   static const unsigned ICmpInst = 46;
   static const unsigned CallInst = 49;
   static const unsigned CmpInst = -1;

#if 0
    static const unsigned AddrSpaceCastInst;
    static const unsigned PtrToIntInst;
    static const unsigned IntToPtrInst;
    static const unsigned FPToSIInst;
    static const unsigned FPToUIInst;
    static const unsigned SIToFPInst;
    static const unsigned UIToFPInst;
    static const unsigned FPExtInst;
    static const unsigned FPTruncInst;
    static const unsigned SExtInst;
    static const unsigned ZExtInst;
    static const unsigned TruncInst;
    static const unsigned UnreachableInst;
    static const unsigned CleanupReturnInst;
    static const unsigned CatchReturnInst;
    static const unsigned CatchPadInst;
    static const unsigned CleanupPadInst;
    static const unsigned CatchSwitchInst;
    static const unsigned ResumeInst;
    static const unsigned InvokeInst;
    static const unsigned IndirectBrInst;
    static const unsigned SwitchInst;
    static const unsigned LandingPadInst;
    static const unsigned PHINode;
    static const unsigned InsertValueInst;
    static const unsigned ExtractValueInst;
    static const unsigned ShuffleVectorInst;
    static const unsigned InsertElementInst;
    static const unsigned ExtractElementInst;
    static const unsigned VAArgInst;
    static const unsigned SelectInst;
    static const unsigned FCmpInst;
    static const unsigned FuncletPadInst;
    static const unsigned GetElementPtrInst;
    static const unsigned CastInst;
    static const unsigned AtomicRMWInst;
    static const unsigned AtomicCmpXchgInst;
    static const unsigned BinaryOperator;
    static const unsigned FenceInst;
    static const unsigned InstrProfValueProfileInst;
    static const unsigned InstrProfIncrementInst;
    static const unsigned VACopyInst;
    static const unsigned VAEndInst;
    static const unsigned GCRelocateInst;
    static const unsigned UnaryInstruction;
    static const unsigned VAStartInst;
    static const unsigned MemMoveInst;
    static const unsigned MemCpyInst;
    static const unsigned MemTransferInst;
    static const unsigned MemSetInst;
    static const unsigned MemIntrinsic;
    static const unsigned DbgValueInst;
    static const unsigned DbgDeclareInst;
    static const unsigned TerminatorInst;
    static const unsigned DbgInfoIntrinsic;
    static const unsigned IntrinsicInst;
#endif

 public:
    OpCodes(){
        //   this->AddrSpaceCastInst = llvm::AddrSpaceCastInst().getOpCode();
        /*
    static const unsigned BitCastInst;
    static const unsigned PtrToIntInst;
    static const unsigned IntToPtrInst;
    static const unsigned FPToSIInst;
    static const unsigned FPToUIInst;
    static const unsigned SIToFPInst;
    static const unsigned UIToFPInst;
    static const unsigned FPExtInst;
    static const unsigned FPTruncInst;
    static const unsigned SExtInst;
    static const unsigned ZExtInst;
    static const unsigned TruncInst;
    static const unsigned UnreachableInst;
    static const unsigned CleanupReturnInst;
    static const unsigned CatchReturnInst;
    static const unsigned CatchPadInst;
    static const unsigned CleanupPadInst;
    static const unsigned CatchSwitchInst;
    static const unsigned ResumeInst;
    static const unsigned InvokeInst;
    static const unsigned IndirectBrInst;
    static const unsigned SwitchInst;
    static const unsigned BranchInst;
    static const unsigned ReturnInst;
    static const unsigned LandingPadInst;
    static const unsigned PHINode;
    static const unsigned InsertValueInst;
    static const unsigned ExtractValueInst;
    static const unsigned ShuffleVectorInst;
    static const unsigned InsertElementInst;
    static const unsigned ExtractElementInst;
    static const unsigned VAArgInst;
    static const unsigned SelectInst;
    static const unsigned CallInst;
    static const unsigned FCmpInst;
    static const unsigned ICmpInst;
    static const unsigned FuncletPadInst;
    static const unsigned CmpInst;
    static const unsigned GetElementPtrInst;
    static const unsigned CastInst;
    static const unsigned AtomicRMWInst;
    static const unsigned AtomicCmpXchgInst;
    static const unsigned BinaryOperator;
    static const unsigned FenceInst;
    static const unsigned StoreInst;
    static const unsigned InstrProfValueProfileInst;
    static const unsigned InstrProfIncrementInst;
    static const unsigned VACopyInst;
    static const unsigned VAEndInst;
    static const unsigned GCRelocateInst;
    static const unsigned UnaryInstruction;
    static const unsigned VAStartInst;
    static const unsigned MemMoveInst;
    static const unsigned LoadInst;
    static const unsigned MemCpyInst;
    static const unsigned MemTransferInst;
    static const unsigned MemSetInst;
    static const unsigned MemIntrinsic;
    static const unsigned AllocaInst;
    static const unsigned DbgValueInst;
    static const unsigned DbgDeclareInst;
    static const unsigned TerminatorInst;
    static const unsigned DbgInfoIntrinsic;
    static const unsigned IntrinsicInst;*/

    }
};
static OpCodes opcodes;

