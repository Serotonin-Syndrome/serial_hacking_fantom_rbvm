#include "RbvmTargetMachine.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target llvm::TheRbvmBackendTarget;

extern "C" void LLVMInitializeRbvmBackendTargetInfo() {
    RegisterTarget<> X(TheRbvmBackendTarget, "c", "RBVM backend", "RBVM");
}

extern "C" void LLVMInitializeRbvmBackendTargetMC() {}
