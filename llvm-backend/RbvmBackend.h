#include "RbvmTargetMachine.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Transforms/Scalar.h"

#include "../vm/opcode.h"

#include <vector>
#include <string>
#include <map>
#include <string.h>

namespace {
    using namespace llvm;

    class RbvmMCAsmInfo : public MCAsmInfo {
    public:
        RbvmMCAsmInfo() { PrivateGlobalPrefix = ""; }
    };

    class RbvmWriter : public FunctionPass, 
                       public InstVisitor<RbvmWriter> {
    private:
        typedef size_t HandleFD, HandleJMP, HandleJZ;

        std::string Mem;
        raw_pwrite_stream &out;
        LoopInfo *LI = nullptr;
        IntrinsicLowering *IL = nullptr;
        const DataLayout *TD = nullptr;
        std::vector<Function *> prototypesToGen;
        std::vector<std::pair<BasicBlock *, size_t> > PostponedJumps;
        std::map<const Value*, size_t> BlockPositions;
        std::map<const Value*, unsigned> Locals;
        unsigned NextAnonValueNumber = 0;
        unsigned ResultReg = 0;
        unsigned NextReg = 0;


    public:
        static char ID;

        explicit RbvmWriter(raw_pwrite_stream &out)
            : FunctionPass(ID), out(out) {}

        virtual StringRef getPassName() const { return "RBVM backend"; }

        /// getAnalysisUsage - This function should be overriden by passes that need
        /// analysis information to do their job.  If a pass specifies that it uses a
        /// particular analysis result to this function, it can then use the
        /// getAnalysis<AnalysisType>() function, below.
        void getAnalysisUsage(AnalysisUsage &) const;

        /// doInitialization - Virtual method overridden by subclasses to do
        /// any necessary initialization before any pass is run.
        virtual bool doInitialization(Module &);

        /// doFinalization - Virtual method overriden by subclasses to do any
        /// necessary clean up after all passes have run.
        virtual bool doFinalization(Module &);

        /// runOnFunction - Virtual method overriden by subclasses to do the
        /// per-function processing of the pass.
        virtual bool runOnFunction(Function &F);

    private:
        void writeAddress(Value *Operand);
        void modifyGlobal(const std::string &Name, int WhatToStore);

        void produce1(uint8_t V) {
            Mem.append((const char *) &V, sizeof(V));
        }

        void produce4(uint32_t V) {
            Mem.append((const char *) &V, sizeof(V));
        }

        void produce8(uint64_t V) {
            Mem.append((const char *) &V, sizeof(V));
        }

        void produce8_D(double V) {
            static_assert(sizeof(V) == 8, "sizeof(double) != 8");
            Mem.append((const char *) &V, sizeof(V));
        }

        void produceString(const std::string &S) {
            produce4(S.size());
            Mem += S;
        }

        HandleFD produceFuncDecl(const std::string &Name, uint64_t nargs) {
            produce1(CMD_FD);
            produceString(Name);
            produce8(nargs);
            const size_t pos = markPosition();
            produce8(0);
            return pos;
        }

        void fixupFD(HandleFD h) {
            fixup8(h, markPosition() - (h + 8));
        }

        void produceAmbigRR(Commands cmd, int R1, int R2) {
            produce1(cmd);
            produce1(0);
            produce1(R1);
            produce1(R2);
        }

        void produceAmbigRC(Commands cmd, int R, uint64_t C) {
            produce1(cmd);
            produce1(1);
            produce1(R);
            produce8(C);
        }

        void produceAmbigRC_D(Commands cmd, int R, double C) {
            produce1(cmd);
            produce1(1);
            produce1(R);
            produce8_D(C);
        }

        void produceSG(const std::string &name, int reg) {
            produce1(Commands::CMD_SG);
            produceString(name);
            produce1(reg);
        }

        void produceGG(const std::string &name, int reg) {
            produce1(Commands::CMD_GG);
            produceString(name);
            produce1(reg);
        }

        void produceCSS_DYN(int r1, int r2) {
            produce1(Commands::CMD_CSS_DYN);
            produce1(r1);
            produce1(r2);
        }

        void produceST_RR(int width, int r1, int r2) {
            switch (width) {
            case 8: produceAmbigRR(Commands::CMD_ST8, r1, r2); break;
            case 16: produceAmbigRR(Commands::CMD_ST16, r1, r2); break;
            case 32: produceAmbigRR(Commands::CMD_ST32, r1, r2); break;
            case 64: produceAmbigRR(Commands::CMD_ST64, r1, r2); break;
            }
        }

        void produceLD_RR(int width, int r1, int r2) {
            switch (width) {
            case 8: produceAmbigRR(Commands::CMD_LD8, r1, r2); break;
            case 16: produceAmbigRR(Commands::CMD_LD16, r1, r2); break;
            case 32: produceAmbigRR(Commands::CMD_LD32, r1, r2); break;
            case 64: produceAmbigRR(Commands::CMD_LD64, r1, r2); break;
            }
        }

        void produceRetR(int r) {
            produce1(Commands::CMD_RET);
            produce1(0);
            produce1(r);
        }

        size_t markPosition() const {
            return Mem.size();
        }

        void fixup8(size_t pos, uint64_t val) {
            memcpy(&Mem[pos], &val, sizeof(val));
        }

        void rememberBlock(BasicBlock *BB) {
            BlockPositions[BB] = markPosition();
        }

        HandleJMP localJMP() {
            const size_t pos = markPosition();
            produce1(Commands::CMD_JMP);
            produce8(0);
            return pos;
        }

        HandleJZ localJZ(int reg) {
            const size_t pos = markPosition();
            produce1(Commands::CMD_JZ);
            produce1(reg);
            produce8(0);
            return pos;
        }

        void fixupLocalJMP(HandleJMP h) {
            fixup8(h + 1, markPosition() - h);
        }

        void fixupLocalJZ(HandleJZ h) {
            fixup8(h + 2, markPosition() - h);
        }

        void produceStraightJump(size_t to) {
            const size_t pos = markPosition();
            produce1(Commands::CMD_JMP);
            produce8(to - pos);
        }

        void postponeJmp(BasicBlock *BB) {
            PostponedJumps.push_back({BB, markPosition()});
            produce1(Commands::CMD_JMP);
            produce8(-1);
        }

        void fixupPostponed() {
            for (const auto &p : PostponedJumps)
                fixup8(p.second + 1, BlockPositions[p.first] - p.second);
        }

        /// releaseMemory() - This member can be implemented by a pass if it wants to
        /// be able to release its memory when it is no longer needed.  The default
        /// behavior of passes is to hold onto memory for the entire duration of their
        /// lifetime (which is the entire compile time).  For pipelined passes, this
        /// is not a big deal because that memory gets recycled every time the pass is
        /// invoked on another program unit.  For IP passes, it is more important to
        /// free memory when it is unused.
        ///
        /// Optionally implement this function to release pass memory when it is no
        /// longer used.
        void releaseMemory();

        void declareGlobalVariable(GlobalVariable*);

        void lowerIntrinsics(Function&);
        void printFunction(Function&);
        void printLoop(Loop*);
        void printBasicBlock(BasicBlock*);

        friend class InstVisitor<RbvmWriter>;

        AllocaInst* isDirectAlloca(Value*) const;
        void writeInstComputationInline(Instruction&);
        bool isAddressExposed(Value*);
        void writeOperandDeref(Value*);
        void writeOperandInternal(Value*);
        void writeOperand(Value*);
        void printConstant(Constant*);
        bool printConstantString(Constant*, unsigned new_reg);
        void printConstantArray(ConstantArray*);
        void printConstantDataSequential(ConstantDataSequential*);

        void writeCastTo(Type*);

        void visitReturnInst(ReturnInst&);
        void visitBranchInst(BranchInst&);
        void visitSwitchInst(SwitchInst&);

        void visitUnreachable(UnreachableInst&) {}
        
        void visitInvokeInst(InvokeInst&) {
            llvm_unreachable("Lowerinvoke pass didn't work!"); 
        }
        
        void visitResumeInst(ResumeInst&) {
            llvm_unreachable("DwarfEHPrepare pass didn't work!"); 
        }
        
        void visitPHINode(PHINode& I) { 
            writeOperand(&I); 
        }
        void visitBinaryOperator(BinaryOperator&);
        void visitICmpInst(ICmpInst&);
        void visitFCmpInst(FCmpInst&);

        void visitCastInst(CastInst&);
        void visitSelectInst(SelectInst&);
        void visitCallInst(CallInst&);

        void visitLoadInst(LoadInst&);
        void visitStoreInst(StoreInst&);
        void printPHICopiesForSuccessor(BasicBlock *CurBlock, BasicBlock *Successor, unsigned Indent);
        void printBranchToBlock(BasicBlock *CurBlock, BasicBlock *Successor, unsigned Indent);
        void writeGEPExpression(Value*, gep_type_iterator, gep_type_iterator);
        void visitGetElementPtrInst(GetElementPtrInst&);
        // void visitVAArgInst (VAArgInst &I);????

        void visitInstruction(Instruction &I) { 
            errs() << "Rbvm Writer cannot digest this instruction!!!!" 
                   << I; 
            llvm_unreachable(0); 
        }
    };
}
