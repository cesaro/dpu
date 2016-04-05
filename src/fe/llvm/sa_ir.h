
namespace dpu {
namespace fe {
namespace llvm {


void outputAlloca( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value*  );
void outputLoad( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value*  );
void outputStore( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value*  );
void outputCall( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value* );
void outputReturn( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value* );
void outputBranch( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value* );
void outputIcmp( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value* );

void outputUnknown( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value* );

} } } // namespace dpu::fe::llvm

