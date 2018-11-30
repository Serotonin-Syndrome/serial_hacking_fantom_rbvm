#ifndef opcode_h_
#define opcode_h_

enum Commands : unsigned char {
    CMD_FD,

    CMD_MOV,

    CMD_GG,
    CMD_SG,
    CMD_CSS,

    CMD_LD8,
    CMD_LD16,
    CMD_LD32,
    CMD_LD64,
    CMD_ST8,
    CMD_ST16,
    CMD_ST32,
    CMD_ST64,
    CMD_LEA,

    CMD_IADD,
    CMD_ISUB,
    CMD_SMUL,
    CMD_UMUL,
    CMD_SREM,
    CMD_UREM,
    CMD_SDIV,
    CMD_UDIV,

    CMD_AND,
    CMD_OR,
    CMD_XOR,
    CMD_SHL,
    CMD_LSHR,
    CMD_ASHR,
    CMD_INEG,

    CMD_FADD,
    CMD_FSUB,
    CMD_FMUL,
    CMD_FDIV,
    CMD_FREM,

    CMD_EQ,
    CMD_NE,

    CMD_SLT,
    CMD_SLE,
    CMD_SGT,
    CMD_SGE,
    CMD_ULT,
    CMD_ULE,
    CMD_UGT,
    CMD_UGE,

    CMD_FEQ,
    CMD_FNE,
    CMD_FLT,
    CMD_FLE,
    CMD_FGT,
    CMD_FGE,

    CMD_JMP,
    CMD_JNZ,
    CMD_JZ,

    CMD_CALL0,
    CMD_CALL1,
    CMD_CALL2,
    CMD_CALL3,
    CMD_CALL4,
    CMD_CALL5,
    CMD_CALL6,
    CMD_CALL7,
    CMD_CALL8,

    CMD_RET,
    CMD_LEAVE,

    CMD_CSS_DYN,


    __CMD_LAST__
};

#endif
