void outputAlloca( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value*  );
void outputLoad( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value*  );
void outputStore( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value*  );
void outputCall( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value* );

void outputUnknown( llvm::Module*, llvm::Instruction*, machine_t*, llvm::Value* );
