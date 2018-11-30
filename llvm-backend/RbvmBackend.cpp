#include "RbvmBackend.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/Host.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/IR/GlobalVariable.h"
#include "compat.h"

#include <assert.h>
#include <stdio.h>

using namespace llvm;

extern "C" void LLVMInitializeRbvmBackendTarget() {
    RegisterTargetMachine<RbvmTargetMachine> X(TheRbvmBackendTarget);
}

static bool isEmptyType(Type *Ty) {
    if (StructType *STy = dyn_cast<StructType>(Ty))
        return std::all_of(STy->element_begin(), STy->element_end(), [](Type *T){ return isEmptyType(T); });

    if (VectorType *VTy = dyn_cast<VectorType>(Ty))
        return VTy->getNumElements() == 0 || isEmptyType(VTy->getElementType());

    if (ArrayType *ATy = dyn_cast<ArrayType>(Ty))
        return ATy->getNumElements() == 0 || isEmptyType(ATy->getElementType());

    return Ty->isVoidTy();
}

static std::string Mangle(const std::string &S) {
    std::string result;

    for (unsigned i = 0, e = S.size(); i != e; ++i)
        if (isalnum(S[i]) || S[i] == '_') {
            result += S[i];
        } 
        else {
            result += '_';
            result += 'A' + (S[i] & 15);
            result += 'A'+ ((S[i] >> 4) & 15);
            result += '_';
        }
    return result;
}

void RbvmWriter::writeCastTo(Type *Ty) {
    if (Ty->isIntegerTy()) {
        IntegerType *ITy = static_cast<IntegerType*>(Ty);
        uint64_t mask = ITy->getBitMask();
        if (mask != UINT64_MAX) {
            auto new_reg = ++NextReg;
            produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
            produceAmbigRC(Commands::CMD_AND, new_reg, mask);
            ResultReg = new_reg;
        }
    }
}

AllocaInst *RbvmWriter::isDirectAlloca(Value *V) const {
    AllocaInst *AI = dyn_cast<AllocaInst>(V);
    if (!AI || AI->isArrayAllocation() || 
        (AI->getParent() != &AI->getParent()->getParent()->getEntryBlock()))
        return nullptr;

    return AI;
}

char RbvmWriter::ID = 0;

bool RbvmWriter::doInitialization(Module &M) {
    TD = new DataLayout(&M);
    IL = new IntrinsicLowering(*TD);
    IL->AddPrototypes(M);
    return false;
}

void RbvmWriter::writeAddress(Value *Operand) {
    writeOperand(Operand);
    Constant* CPV = dyn_cast<Constant>(Operand);

    if (CPV && CPV->getType()->getTypeID() != Type::ArrayTyID) {
        auto new_reg = ++NextReg;
        produceCSS_DYN(new_reg, ResultReg);
        ResultReg = new_reg;
    }
}

void RbvmWriter::modifyGlobal(const std::string &Name, int WhatToStore) {
    auto new_reg = ++NextReg;
    produceCSS_DYN(new_reg, WhatToStore);
    produceSG(Name, new_reg);
}

void RbvmWriter::declareGlobalVariable(GlobalVariable *I) {
    if (I->isDeclaration() || isEmptyType(I->getType()->getPointerElementType()))
        return;

    std::string name = Mangle(I->getName());
    if (!I->getInitializer()->isNullValue()) {
        writeAddress(I->getInitializer());
        produceSG(name, ResultReg);
    } 
    else {
        auto new_reg = ++NextReg;
        produceAmbigRC(Commands::CMD_MOV, new_reg, 0);
        produceSG(name, new_reg);
        ResultReg = new_reg;
    }
}

bool RbvmWriter::doFinalization(Module &M) {
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
        declareGlobalVariable(&*I);

    produceGG("main", 1);
    produce1(Commands::CMD_CALL0);
    produce1(1);

    out << Mem;
    return false;
}

void RbvmWriter::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.setPreservesCFG();
}

void RbvmWriter::releaseMemory()
{
}

bool RbvmWriter::runOnFunction(Function &F) {
    if (F.hasAvailableExternallyLinkage())
        return false;

    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    lowerIntrinsics(F);
    printFunction(F);
    LI = nullptr;
    return true;
}

void RbvmWriter::lowerIntrinsics(Function &F) {
    for (auto &BB : F) {
        for (auto I = BB.begin(), E = BB.end(); I != E;) {
            if (CallInst *CI = dyn_cast<CallInst>(I++)) {
                if (Function *G = CI->getCalledFunction()) {
                    switch (G->getIntrinsicID()) {
                    case Intrinsic::not_intrinsic:
                    case Intrinsic::vastart:
                    case Intrinsic::vacopy:
                    case Intrinsic::vaend:
                    case Intrinsic::returnaddress:
                    case Intrinsic::frameaddress:
                    case Intrinsic::setjmp:
                    case Intrinsic::longjmp:
                    case Intrinsic::sigsetjmp:
                    case Intrinsic::siglongjmp:
                    case Intrinsic::trap:
                    case Intrinsic::stackprotector:
                    case Intrinsic::dbg_value:
                    case Intrinsic::dbg_declare:
                        break;
                    default:
                        Instruction *Before = nullptr;
                        if (CI != &BB.front())
                            Before = &*std::prev(BasicBlock::iterator(CI));

                        IL->LowerIntrinsicCall(CI);
                        if (Before) {
                            I = BasicBlock::iterator(Before);
                            ++I;
                        } else
                            I = BB.begin();

                        if (CallInst *Call = dyn_cast<CallInst>(I)) {
                            if (Function *NewF = Call->getCalledFunction()) {
                                if (!NewF->isDeclaration()) {
                                    prototypesToGen.push_back(NewF);
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

void RbvmWriter::writeInstComputationInline(Instruction &I) {
    visit(&I);
    writeCastTo(I.getType());
}

void RbvmWriter::printBasicBlock(BasicBlock *BB) {
    rememberBlock(BB);

    for (BasicBlock::iterator II = BB->begin(), E = --BB->end(); II != E; ++II) {
        if (!isDirectAlloca(&*II)) {
            writeInstComputationInline(*II);
            if (!isEmptyType(II->getType())) {
                produceAmbigRR(Commands::CMD_MOV, Locals[&*II], ResultReg);
            }
        }
    }
    visit(*BB->getTerminator());
}

void RbvmWriter::printLoop(Loop *L) {
    const size_t begin_pos = markPosition();

    for (BasicBlock *BB : L->getBlocks()) {
        Loop *BBLoop = LI->getLoopFor(BB);
        if (BBLoop == L)
            printBasicBlock(BB);
        else if (BB == BBLoop->getHeader() && BBLoop->getParentLoop() == L)
            printLoop(BBLoop);
    }

    produceStraightJump(begin_pos);
}

void RbvmWriter::printFunction(Function &F) {
    assert(!F.isDeclaration());

    HandleFD h = produceFuncDecl(F.getName(),F.getFunctionType()->getNumParams());

    for (auto &ArgName : F.args()) {
        Locals[&ArgName] = ++NextReg;
    }
    for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
        if (!isEmptyType(I->getType())) {
            Locals[&*I] = ++NextReg;
        }
    }

    for (BasicBlock &BB_ref : F) {
        BasicBlock *BB = &BB_ref;
        if (Loop *L = LI->getLoopFor(BB)) {
            if (L->getHeader() == BB && L->getParentLoop() == 0) {
                printLoop(L);
            }
        } 
        else
            printBasicBlock(BB);
    }

    fixupFD(h);
    fixupPostponed();

    PostponedJumps.clear();
    BlockPositions.clear();
    Locals.clear();
    NextAnonValueNumber = 0;
    NextReg = 0;
    ResultReg = 0;
}

void RbvmWriter::visitLoadInst(LoadInst &I) {
    Value *Operand = I.getOperand(0);
    if (isAddressExposed(Operand))
        writeOperandInternal(Operand);
    else {
        writeOperand(Operand);
        auto where = ++NextReg;

        auto width = Operand->getType()->getPointerElementType()->getPrimitiveSizeInBits();
        produceLD_RR(width, where, ResultReg);

        ResultReg = where;
    }
}

void RbvmWriter::visitStoreInst(StoreInst &I) {
    Value *Pointer = I.getPointerOperand();
    writeOperand(I.getOperand(0));
    auto whatToStore = ResultReg;

    if (isa<GlobalValue>(Pointer)) {
        if (GlobalAlias *GA = dyn_cast<GlobalAlias>(Pointer)) {
            Pointer = GA->getAliasee();
        }
        modifyGlobal(Pointer->getName(), whatToStore);
    } else if (isDirectAlloca(Pointer)) {
        writeOperandInternal(Pointer);
        produceAmbigRR(Commands::CMD_MOV, ResultReg, whatToStore);
    } else {
        writeOperandInternal(Pointer);
        auto width = Pointer->getType()->getPointerElementType()->getPrimitiveSizeInBits();
        produceST_RR(width, ResultReg, whatToStore);
    }
}

void RbvmWriter::visitReturnInst(ReturnInst &I) {
    if (I.getNumOperands()) {
        writeOperand(I.getOperand(0));
        produceRetR(ResultReg);
        return;
    }
    produce1(Commands::CMD_LEAVE);
}

void RbvmWriter::writeOperandInternal(Value *Operand) {
    Constant* CPV = dyn_cast<Constant>(Operand);

    if (CPV && !isa<GlobalValue>(CPV))
        printConstant(CPV);
    else if (isa<GlobalValue>(Operand)) {
        if (GlobalAlias *GA = dyn_cast<GlobalAlias>(Operand)) {
            Operand = GA->getAliasee();
        }
        ResultReg = ++NextReg;
        produceGG(Mangle(Operand->getName()), ResultReg);
    } 
    else
        ResultReg = Locals[Operand];
}

bool RbvmWriter::isAddressExposed(Value *V) {
    return isDirectAlloca(V);
}

void RbvmWriter::writeOperand(Value *Operand) {
    writeOperandInternal(Operand);
    if (isAddressExposed(Operand)) {
        auto prev = ResultReg;
        ResultReg = ++NextReg;

        produce1(Commands::CMD_LEA);
        produce1(ResultReg);
        produce1(prev);
    }
}

bool RbvmTargetMachine::addPassesToEmitFile (
        PassManagerBase &PM,
        raw_pwrite_stream &out,
#if ! LESSER_LLVM
        raw_pwrite_stream *DwoOut,
#endif
        CodeGenFileType FileType,
        bool DisableVerify,
        MachineModuleInfo *MMI) {
    
    if (FileType != TargetMachine::CGFT_AssemblyFile)
        return true;

    PM.add(createGCLoweringPass());
    PM.add(createLowerInvokePass());
    PM.add(createCFGSimplificationPass());
    PM.add(new RbvmWriter(out));
    return false;
}


static Commands GetMnemonics(unsigned opcode) {
    switch (opcode) {
    case Instruction::Add:      return Commands::CMD_IADD;
    case Instruction::FAdd:     return Commands::CMD_FADD;
    case Instruction::Sub:      return Commands::CMD_ISUB;
    case Instruction::FSub:     return Commands::CMD_FSUB;
    case Instruction::Mul:      return Commands::CMD_SMUL;
    case Instruction::FMul:     return Commands::CMD_FMUL;
    case Instruction::URem:     return Commands::CMD_UREM;
    case Instruction::SRem:     return Commands::CMD_SREM;
    case Instruction::FRem:     return Commands::CMD_FREM;
    case Instruction::UDiv:     return Commands::CMD_UDIV;
    case Instruction::SDiv:     return Commands::CMD_SDIV;
    case Instruction::FDiv:     return Commands::CMD_FDIV;
    case Instruction::And:      return Commands::CMD_AND;
    case Instruction::Or:       return Commands::CMD_OR;
    case Instruction::Xor:      return Commands::CMD_XOR;
    case Instruction::Shl :     return Commands::CMD_SHL;
    case Instruction::LShr:     return Commands::CMD_LSHR;
    case Instruction::AShr:     return Commands::CMD_ASHR;
    default:
                            errs() << "Invalid operator type!" << opcode;
                            llvm_unreachable(0);
    }
}


void RbvmWriter::visitBinaryOperator(BinaryOperator &I) {
    assert(!I.getType()->isPointerTy());
    writeOperand(I.getOperand(0));
    auto new_reg = ++NextReg;
    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);

    writeOperand(I.getOperand(1));

    produceAmbigRR(GetMnemonics(I.getOpcode()), new_reg, ResultReg);
    ResultReg = new_reg;

    writeCastTo(I.getType());
}


static Commands getPredicate(unsigned opcode) {
    switch (opcode) {
    case ICmpInst::ICMP_EQ:  return Commands::CMD_EQ;
    case ICmpInst::ICMP_NE:  return Commands::CMD_NE;
    case ICmpInst::ICMP_ULE: return Commands::CMD_ULE;
    case ICmpInst::ICMP_SLE: return Commands::CMD_SLE;
    case ICmpInst::ICMP_UGE: return Commands::CMD_UGE;
    case ICmpInst::ICMP_SGE: return Commands::CMD_SGE;
    case ICmpInst::ICMP_ULT: return Commands::CMD_ULT;
    case ICmpInst::ICMP_SLT: return Commands::CMD_SLT;
    case ICmpInst::ICMP_UGT: return Commands::CMD_UGT;
    case ICmpInst::ICMP_SGT: return Commands::CMD_SGT;
    default:
                errs() << "Invalid icmp predicate!" << opcode;
                llvm_unreachable(0);
    }
}


void RbvmWriter::visitICmpInst(ICmpInst &I) {
    writeOperand(I.getOperand(0));
    auto new_reg = ++NextReg;
    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);

    writeOperand(I.getOperand(1));

    produceAmbigRR(getPredicate(I.getPredicate()), new_reg, ResultReg);
    ResultReg = new_reg;
}


void RbvmWriter::visitFCmpInst(FCmpInst &I) {
    writeOperand(I.getOperand(0));
    auto new_reg = ++NextReg;
    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);

    writeOperand(I.getOperand(1));

    produceAmbigRR(getPredicate(I.getPredicate()), new_reg, ResultReg);
    ResultReg = new_reg;
}


void RbvmWriter::printPHICopiesForSuccessor(BasicBlock *CurBlock, BasicBlock *Successor, unsigned Indent) {
    for (auto I = Successor->begin(); isa<PHINode>(I); ++I) {
        PHINode *PN = cast<PHINode>(I);
        Value *IV = PN->getIncomingValueForBlock(CurBlock);
        if (!isa<UndefValue>(IV) && !isEmptyType(IV->getType())) {
            writeOperand(IV);
            if (isa<GlobalValue>(&*I))
                modifyGlobal(Mangle(I->getName()), ResultReg);
            else
                produceAmbigRR(Commands::CMD_MOV, Locals[&*I], ResultReg);
        }
    }
}

void RbvmWriter::printBranchToBlock(BasicBlock *CurBB, BasicBlock *Succ, unsigned Indent) {
    postponeJmp(Succ);
}

void RbvmWriter::visitBranchInst(BranchInst &I) {
    if (I.isConditional()) {
        writeOperand(I.getCondition());
        HandleJZ jz = localJZ(ResultReg);

        printPHICopiesForSuccessor(I.getParent(), I.getSuccessor(0), 4);
        printBranchToBlock(I.getParent(), I.getSuccessor(0), 4);
        HandleJMP jmp = localJMP();
        fixupLocalJZ(jz);
        printPHICopiesForSuccessor(I.getParent(), I.getSuccessor(1), 4);
        printBranchToBlock(I.getParent(), I.getSuccessor(1), 4);
        fixupLocalJMP(jmp);
    } else {
        printPHICopiesForSuccessor(I.getParent(), I.getSuccessor(0), 0);
        printBranchToBlock(I.getParent(), I.getSuccessor(0), 0);
    }
}


void RbvmWriter::visitSwitchInst(SwitchInst &SI) {
    Value* Cond = SI.getCondition();

    if (SI.getNumCases() == 0) {
        printPHICopiesForSuccessor(SI.getParent(), SI.getDefaultDest(), 4);
        printBranchToBlock(SI.getParent(), SI.getDefaultDest(), 4);
    } else {
        for (auto i = SI.case_begin(), e = SI.case_end(); i != e; ++i) {
            ConstantInt* CaseVal = i->getCaseValue();
            BasicBlock* Succ = i->getCaseSuccessor();

            ICmpInst *icmp = new ICmpInst(CmpInst::ICMP_EQ, Cond, CaseVal);
            visitICmpInst(*icmp);
            delete icmp;

            HandleJZ jz = localJZ(ResultReg);

            printPHICopiesForSuccessor(SI.getParent(), Succ, 4);
            printBranchToBlock(SI.getParent(), Succ, 4);

            fixupLocalJZ(jz);
        }
        printPHICopiesForSuccessor(SI.getParent(), SI.getDefaultDest(), 4);
        printBranchToBlock(SI.getParent(), SI.getDefaultDest(), 4);
    }
}

bool RbvmWriter::printConstantString(Constant *C, unsigned new_reg) {
    ConstantDataSequential *CDS = dyn_cast<ConstantDataSequential>(C);
    if (!CDS || !CDS->isCString())
        return false;

    produce1(Commands::CMD_CSS);
    produceString(CDS->getAsString());
    produce1(new_reg);

    return true;
}


void RbvmWriter::printConstant(Constant *CPV) {
    auto new_reg = ++NextReg;

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(CPV)) {
        assert(CE->getType()->isIntegerTy() || CE->getType()->isFloatingPointTy() || CE->getType()->isPointerTy());
        switch (CE->getOpcode()) {
            case Instruction::Trunc:
            case Instruction::ZExt:
            case Instruction::SExt:
            case Instruction::FPTrunc:
            case Instruction::FPExt:
            case Instruction::UIToFP:
            case Instruction::SIToFP:
            case Instruction::FPToUI:
            case Instruction::FPToSI:
            case Instruction::PtrToInt:
            case Instruction::IntToPtr:
            case Instruction::BitCast: {
                    printConstant(CE->getOperand(0));

                    if (CE->getType() == Type::getInt1Ty(CPV->getContext()) && CE->getOpcode() == Instruction::SExt) {
                        produce1(Commands::CMD_INEG);
                        produce1(ResultReg);
                    }
                    if (CE->getType() == Type::getInt1Ty(CPV->getContext()) &&
                        (CE->getOpcode() == Instruction::Trunc ||
                         CE->getOpcode() == Instruction::FPToUI ||
                         CE->getOpcode() == Instruction::FPToSI ||
                         CE->getOpcode() == Instruction::PtrToInt))
                            produceAmbigRC(Commands::CMD_AND, ResultReg, 1);
                    
                    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
                }
                break;

            case Instruction::GetElementPtr:
                writeGEPExpression(CE->getOperand(0), gep_type_begin(CPV), gep_type_end(CPV));
                produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
                break;
            case Instruction::Select: {
                    printConstant(CE->getOperand(0));
                    HandleJZ jz = localJZ(ResultReg);

                    printConstant(CE->getOperand(1));
                    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
                    HandleJMP jmp = localJMP();

                    fixupLocalJZ(jz);
                    printConstant(CE->getOperand(2));
                    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);

                    fixupLocalJMP(jmp);
                }
                break;
            case Instruction::Add:
            case Instruction::FAdd:
            case Instruction::Sub:
            case Instruction::FSub:
            case Instruction::Mul:
            case Instruction::FMul:
            case Instruction::SDiv:
            case Instruction::UDiv:
            case Instruction::FDiv:
            case Instruction::URem:
            case Instruction::SRem:
            case Instruction::FRem:
            case Instruction::And:
            case Instruction::Or:
            case Instruction::Xor:
            case Instruction::Shl:
            case Instruction::LShr:
            case Instruction::AShr: {
                printConstant(CE->getOperand(0));
                produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
                printConstant(CE->getOperand(1));
                produceAmbigRR(GetMnemonics(CE->getOpcode()), new_reg, ResultReg);
                break;
            }
            case Instruction::ICmp:
            case Instruction::FCmp: {
                printConstant(CE->getOperand(0));
                produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
                printConstant(CE->getOperand(1));
                produceAmbigRR(getPredicate(CE->getOpcode()), new_reg, ResultReg);
            }
                break;

            default:
                    errs() << "RbvmWriter Error: Unhandled constant expression: "
                           << *CE << "\n";
                    llvm_unreachable(0);
        }
    } else if (isa<UndefValue>(CPV) && CPV->getType()->isSingleValueType()) {
        Constant *Zero = Constant::getNullValue(CPV->getType());
        printConstant(Zero);
        produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
    } else if (ConstantInt *CI = dyn_cast<ConstantInt>(CPV)) {
        Type* Ty = CI->getType();
        if (Ty == Type::getInt1Ty(CPV->getContext()))
            produceAmbigRC(Commands::CMD_MOV, new_reg, !!CI->getZExtValue());
        else {
            const int64_t val = CI->getSExtValue();
            produceAmbigRC(Commands::CMD_MOV, new_reg, val);
            ResultReg = new_reg;
            writeCastTo(Ty);
        }
    } else 
        switch (CPV->getType()->getTypeID()) {
            case Type::FloatTyID:
            case Type::DoubleTyID:
            case Type::X86_FP80TyID:
            case Type::PPC_FP128TyID:
            case Type::FP128TyID: {
                ConstantFP *FPC = cast<ConstantFP>(CPV);
                double V;
                if (FPC->getType() == Type::getFloatTy(CPV->getContext()))
                    V = FPC->getValueAPF().convertToFloat();
                else if (FPC->getType() == Type::getDoubleTy(CPV->getContext()))
                    V = FPC->getValueAPF().convertToDouble();
                else {
                    APFloat Tmp = FPC->getValueAPF();
                    bool LosesInfo;
                    Tmp.convert(APFloat::IEEEdouble(), APFloat::rmTowardZero, &LosesInfo);
                    V = Tmp.convertToDouble();
                }

                produceAmbigRC_D(Commands::CMD_MOV, new_reg, V);
                break;
            }
            case Type::ArrayTyID: {                                                                             // TODO
                if (printConstantString(CPV, new_reg))
                    break;
                break;
            }

            case Type::StructTyID: {}                                                                           // TODO
            break;

            case Type::PointerTyID:
                if (isa<ConstantPointerNull>(CPV)) {
                    produceAmbigRC(Commands::CMD_MOV, new_reg, 0);
                    break;
                } else if (GlobalValue *GV = dyn_cast<GlobalValue>(CPV)) {
                    writeOperand(GV);
                    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
                    break;
                }
            // fall thru
            default:
                        errs() << "Unknown constant type: " << *CPV << "\n";
                        llvm_unreachable(0);
        }

    ResultReg = new_reg;
}

void RbvmWriter::visitCastInst(CastInst &I) {
    Type *DstTy = I.getType();

    writeOperand(I.getOperand(0));

    if (DstTy == Type::getInt1Ty(I.getContext()) &&
            (I.getOpcode() == Instruction::Trunc ||
             I.getOpcode() == Instruction::FPToUI ||
             I.getOpcode() == Instruction::FPToSI ||
             I.getOpcode() == Instruction::PtrToInt)) {

        auto new_reg = ++NextReg;
        produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
        produceAmbigRC(Commands::CMD_AND, new_reg, 1);
        ResultReg = new_reg;
    } else
        writeCastTo(DstTy);
}

void RbvmWriter::visitSelectInst(SelectInst &I) {
    auto new_reg = ++NextReg;

    writeOperand(I.getCondition());
    HandleJZ jz = localJZ(ResultReg);

    writeOperand(I.getTrueValue());
    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
    HandleJMP jmp = localJMP();

    fixupLocalJZ(jz);
    writeOperand(I.getFalseValue());
    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
    fixupLocalJMP(jmp);

    ResultReg = new_reg;
    writeCastTo(I.getType());
}

void RbvmWriter::writeGEPExpression(Value *Ptr, gep_type_iterator B, gep_type_iterator E) {
    writeOperand(Ptr);
    auto new_reg = ++NextReg;
    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);
    auto tmp_reg = ++NextReg;

    for (gep_type_iterator I = B; I != E; ++I) {
        writeOperand(I.getOperand());
        Type *IntoT = I.getIndexedType();
        unsigned width = IntoT->getPrimitiveSizeInBits();
        if (width && width % 8 == 0 && width != 8) {
            produceAmbigRR(Commands::CMD_MOV, tmp_reg, ResultReg);
            produceAmbigRC(Commands::CMD_UMUL, tmp_reg, width / 8);
            produceAmbigRR(Commands::CMD_IADD, new_reg, tmp_reg);
        } else
            produceAmbigRR(Commands::CMD_IADD, new_reg, ResultReg);
    }
    ResultReg = new_reg;
}

void RbvmWriter::visitGetElementPtrInst(GetElementPtrInst &I) {
    writeGEPExpression(I.getPointerOperand(), gep_type_begin(I), gep_type_end(I));
}

void RbvmWriter::visitCallInst(CallInst &I) {
    Value *Callee = I.getCalledValue();
    writeOperand(Callee);
    auto new_reg = ++NextReg;
    produceAmbigRR(Commands::CMD_MOV, new_reg, ResultReg);

    unsigned ArgNo = 0;
    CallSite CS(&I);
    CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();

    std::vector<int> RegArgs;

    for (; AI != AE; ++AI, ++ArgNo) {
        writeOperand(*AI);
        if (I.getAttributes().hasAttribute(ArgNo + 1, Attribute::ByVal)) {
            auto new_reg = ++NextReg;
            produceLD_RR(64, new_reg, ResultReg);
            RegArgs.push_back(new_reg);
        } 
        else
            RegArgs.push_back(ResultReg);
    }

    produce1(Commands::CMD_CALL0 + RegArgs.size());
    produce1(new_reg);
    
    for (int R : RegArgs) 
        produce1(R);

    ResultReg = new_reg;
}
