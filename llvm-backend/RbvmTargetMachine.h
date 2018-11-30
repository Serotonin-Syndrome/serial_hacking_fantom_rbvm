#pragma once

#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/DataLayout.h"
#include "compat.h"

namespace llvm {

struct RbvmTargetMachine : public TargetMachine {
    RbvmTargetMachine(const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
                      const TargetOptions &Options, Optional<Reloc::Model> RM,
                      Optional<CodeModel::Model> CM, CodeGenOpt::Level OL, bool JIT)
                    : TargetMachine(T, "", TT, CPU, FS, Options) {}

    bool addPassesToEmitFile(
        PassManagerBase &PM, raw_pwrite_stream &Out,
#if ! LESSER_LLVM
        raw_pwrite_stream *DwoOut,
#endif
        CodeGenFileType FileType, 
        bool DisableVerify = true, 
        MachineModuleInfo* MMI = nullptr) override;

};

extern Target TheRbvmBackendTarget;

} // End llvm namespace

