#include <tuple>
#include <array>
#include <string.h>

#include "opcode.h"
#include "reader.h"


static std::array<const char*, __CMD_LAST__> opcode_names = {};

static void init_opcode_names() {
// this is auto-generated
    opcode_names[CMD_FD] = "fd";
    opcode_names[CMD_MOV] = "mov";
    opcode_names[CMD_GG] = "gg";
    opcode_names[CMD_SG] = "sg";
    opcode_names[CMD_CSS] = "css";
    opcode_names[CMD_LD8] = "ld8";
    opcode_names[CMD_LD16] = "ld16";
    opcode_names[CMD_LD32] = "ld32";
    opcode_names[CMD_LD64] = "ld64";
    opcode_names[CMD_ST8] = "st8";
    opcode_names[CMD_ST16] = "st16";
    opcode_names[CMD_ST32] = "st32";
    opcode_names[CMD_ST64] = "st64";
    opcode_names[CMD_LEA] = "lea";
    opcode_names[CMD_IADD] = "iadd";
    opcode_names[CMD_ISUB] = "isub";
    opcode_names[CMD_SMUL] = "smul";
    opcode_names[CMD_UMUL] = "umul";
    opcode_names[CMD_SREM] = "srem";
    opcode_names[CMD_UREM] = "urem";
    opcode_names[CMD_SDIV] = "sdiv";
    opcode_names[CMD_UDIV] = "udiv";
    opcode_names[CMD_AND] = "and";
    opcode_names[CMD_OR] = "or";
    opcode_names[CMD_XOR] = "xor";
    opcode_names[CMD_SHL] = "shl";
    opcode_names[CMD_LSHR] = "lshr";
    opcode_names[CMD_ASHR] = "ashr";
    opcode_names[CMD_INEG] = "ineg";
    opcode_names[CMD_FADD] = "fadd";
    opcode_names[CMD_FSUB] = "fsub";
    opcode_names[CMD_FMUL] = "fmul";
    opcode_names[CMD_FDIV] = "fdiv";
    opcode_names[CMD_FREM] = "frem";
    opcode_names[CMD_EQ] = "eq";
    opcode_names[CMD_NE] = "ne";
    opcode_names[CMD_SLT] = "slt";
    opcode_names[CMD_SLE] = "sle";
    opcode_names[CMD_SGT] = "sgt";
    opcode_names[CMD_SGE] = "sge";
    opcode_names[CMD_ULT] = "ult";
    opcode_names[CMD_ULE] = "ule";
    opcode_names[CMD_UGT] = "ugt";
    opcode_names[CMD_UGE] = "uge";
    opcode_names[CMD_FEQ] = "feq";
    opcode_names[CMD_FNE] = "fne";
    opcode_names[CMD_FLT] = "flt";
    opcode_names[CMD_FLE] = "fle";
    opcode_names[CMD_FGT] = "fgt";
    opcode_names[CMD_FGE] = "fge";
    opcode_names[CMD_JMP] = "jmp";
    opcode_names[CMD_JNZ] = "jnz";
    opcode_names[CMD_JZ] = "jz";
    opcode_names[CMD_CALL0] = "call0";
    opcode_names[CMD_CALL1] = "call1";
    opcode_names[CMD_CALL2] = "call2";
    opcode_names[CMD_CALL3] = "call3";
    opcode_names[CMD_CALL4] = "call4";
    opcode_names[CMD_CALL5] = "call5";
    opcode_names[CMD_CALL6] = "call6";
    opcode_names[CMD_CALL7] = "call7";
    opcode_names[CMD_CALL8] = "call8";
    opcode_names[CMD_RET] = "ret";
    opcode_names[CMD_LEAVE] = "leave";
    opcode_names[CMD_CSS_DYN] = "css_dyn";
// 
}

template <typename T>
void st(const char* bytecode, unsigned& i) {
    auto has_const = *(unsigned char*)(bytecode + i++);

    if (has_const) {
        auto value_ptr = *(T**)(bytecode + i);
        i += sizeof(T*);

        auto r2 = *(unsigned char*)(bytecode + i++);
        printf("st%d %p, R%d\n", (int) sizeof(T) * 8, (void *) value_ptr, (int) r2);
    }
    else {
        auto r1 = *(unsigned char*)(bytecode + i++);
        auto r2 = *(unsigned char*)(bytecode + i++);
        printf("st%d R%d, R%d\n", (int) sizeof(T) * 8, (int) r1, (int) r2);
    }
}


template <typename T>
void ld(const char* bytecode, unsigned& i) {
    auto has_const = *(unsigned char*)(bytecode + i++);

    if (has_const) {
        auto r2 = *(unsigned char*)(bytecode + i++);

        auto value_ptr = *(T**)(bytecode + i);
        i += sizeof(T*);

        printf("ld%d R%d, %p\n", (int) sizeof(T) * 8, (int) r2, (void *) value_ptr);
    }
    else {
        auto r1 = *(unsigned char*)(bytecode + i++);
        auto r2 = *(unsigned char*)(bytecode + i++);
        printf("ld%d R%d, R%d\n", (int) sizeof(T) * 8, (int) r1, (int) r2);
    }
}


int main(int argc, char** argv) {
    init_opcode_names();
    const char* bytecode = nullptr;
    size_t size = 0;

    if (argc < 2)
        std::tie(bytecode, size) = read_text(stdin);
    else {
        FILE* file = fopen(argv[1], "rb");
        if (!file)
            PANIC();
        std::tie(bytecode, size) = read_text(file);
    }

    unsigned i = 0;

    while (i < size) {
        auto command = *(unsigned char*)(bytecode + i++);

        switch (command) {
// function declaration
            case CMD_FD: {
                auto len = *(uint32_t*)(bytecode + i);
                i += sizeof(uint32_t);

                std::string name(bytecode + i, bytecode + i + len);
                i += len;

                auto nargs = *(uint64_t*)(bytecode + i);
                i += sizeof(uint64_t);

                auto nskip = *(uint64_t*)(bytecode + i);
                i += sizeof(uint64_t);

                printf("fd \"%s\", %d, %d\n", name.c_str(), (int) nargs, (int) nskip);

                break;
            }
// globals
            case CMD_GG: {
                auto len = *(uint32_t*)(bytecode + i);
                i += sizeof(uint32_t);

                std::string name(bytecode + i, bytecode + i + len);
                i += len;

                auto r = *(unsigned char*)(bytecode + i);
                ++i;

                printf("gg \"%s\", R%d\n", name.c_str(), (int) r);
                break;
            }
            case CMD_SG: {
                auto len = *(uint32_t*)(bytecode + i);
                i += sizeof(uint32_t);

                std::string name(bytecode + i, bytecode + i + len);
                i += len;

                auto r = *(unsigned char*)(bytecode + i);
                ++i;
                printf("sg \"%s\", R%d\n", name.c_str(), (int) r);

                break;
            }
            case CMD_CSS: {
                // reading <str>
                auto len = *(uint32_t*)(bytecode + i);
                i += sizeof(uint32_t);

                std::string name(bytecode + i, bytecode + i + len);
                i += len;

                auto r = *(unsigned char*)(bytecode + i);
                ++i;

                printf("css \"%s\", R%d\n", name.c_str(), (int) r);
                break;
            }
            case CMD_CSS_DYN: {
                auto r1 = *(unsigned char*)(bytecode + i++);
                auto r2 = *(unsigned char*)(bytecode + i++);
                
                printf("css_dyn R%d R%d\n", (int)r1, (int)r2);

                break;
            }
// ptrs
            case CMD_LD8: ld<uint8_t>(bytecode, i); break;
            case CMD_LD16: ld<uint16_t>(bytecode, i); break;
            case CMD_LD32: ld<uint32_t>(bytecode, i); break;
            case CMD_LD64: ld<uint64_t>(bytecode, i); break;

            case CMD_ST8: {
                st<uint8_t>(bytecode, i);
                break;
            }
            case CMD_ST16: {
                st<uint16_t>(bytecode, i);
                break;
            }
            case CMD_ST32: {
                st<uint32_t>(bytecode, i);
                break;
            }
            case CMD_ST64: {
                st<uint64_t>(bytecode, i);
                break;
            }
            case CMD_LEA: {
                auto r1 = *(unsigned char*)(bytecode + i++);
                auto r2 = *(unsigned char*)(bytecode + i++);
                printf("lea R%d, R%d\n", (int) r1, (int) r2);
                break;
            }
// integer arithmetic
            case CMD_INEG: {
                auto r = *(unsigned char*)(bytecode + i++);
                printf("ineg R%d\n", (int) r);
                break;
            }
            case CMD_IADD:
            case CMD_ISUB:
            case CMD_SMUL:
            case CMD_SREM:
            case CMD_SDIV:
            case CMD_SLT:
            case CMD_SLE:
            case CMD_SGT:
            case CMD_SGE: {
                auto has_const = *(unsigned char*)(bytecode + i++);
                auto r1 = *(unsigned char*)(bytecode + i++);

                if (has_const) {
                    auto value = *(int64_t*)(bytecode + i);
                    printf("%s R%d, %d\n", opcode_names[command], (int) r1, (int) value);
                    i += sizeof(int64_t);
                }
                else {
                    auto r2 = *(unsigned char*)(bytecode + i++);
                    printf("%s R%d, R%d\n", opcode_names[command], (int) r1, (int) r2);
                }
                break;
            }
            case CMD_MOV:
            case CMD_UMUL:
            case CMD_UREM:
            case CMD_UDIV:
            case CMD_OR:
            case CMD_AND:
            case CMD_XOR:
            case CMD_SHL:
            case CMD_ASHR:
            case CMD_EQ:
            case CMD_NE:
            case CMD_ULT:
            case CMD_ULE:
            case CMD_UGT:
            case CMD_UGE: {
                auto has_const = *(unsigned char*)(bytecode + i++);
                auto r1 = *(unsigned char*)(bytecode + i++);

                if (has_const) {
                    auto value = *(uint64_t*)(bytecode + i);
                    printf("%s R%d, %d\n", opcode_names[command], (int) r1, (int) value);
                    i += sizeof(uint64_t); 
                }
                else {
                    auto r2 = *(unsigned char*)(bytecode + i++);
                    printf("%s R%d, R%d\n", opcode_names[command], (int) r1, (int) r2);
                }
                break;
            }
            case CMD_LSHR:  {
                auto has_const = *(unsigned char*)(bytecode + i++);
                auto r1 = *(unsigned char*)(bytecode + i++);

                if (has_const) {
                    auto value = *(int32_t*)(bytecode + i);
                    printf("%s R%d, %d\n", opcode_names[command], (int) r1, (int) value);
                    i += sizeof(int32_t);
                }
                else {
                    auto r2 = *(unsigned char*)(bytecode + i++);
                    printf("%s R%d, R%d\n", opcode_names[command], (int) r1, (int) r2);
                }
                break;
            }
            case CMD_FADD:
            case CMD_FSUB:
            case CMD_FMUL:
            case CMD_FDIV:
            case CMD_FREM:
            case CMD_FEQ:
            case CMD_FNE:
            case CMD_FLT:
            case CMD_FLE:
            case CMD_FGT:
            case CMD_FGE: {
                auto has_const = *(unsigned char*)(bytecode + i++);
                auto r1 = *(unsigned char*)(bytecode + i++);

                if (has_const) {
                    auto value = *(double*)(bytecode + i);
                    printf("%s R%d, %f\n", opcode_names[command], (int) r1, (double) value);
                    i += sizeof(double);
                }
                else {
                    auto r2 = *(unsigned char*)(bytecode + i++);
                    printf("%s R%d, R%d\n", opcode_names[command], (int) r1, (int) r2);
                }
                break;
            }
// jumps
            case CMD_JMP: {
                auto offset = *(int64_t*)(bytecode + i);
                printf("jmp %d\n", (int) offset);
                i += sizeof(int64_t);
                break;
            }
            case CMD_JNZ: {
                auto r = *(unsigned char*)(bytecode + i++);
                auto offset = *(int64_t*)(bytecode + i);
                printf("jnz R%d, %d\n", (int)r, (int)offset);
                i += sizeof(int64_t);
                break;
            }
            case CMD_JZ: {
                auto r = *(unsigned char*)(bytecode + i++);
                auto offset = *(int64_t*)(bytecode + i);
                printf("jz R%d, %d\n", (int) r, (int) offset);
                i += sizeof(int64_t);
                break;
            }
// call
            case CMD_CALL0:
            case CMD_CALL1:
            case CMD_CALL2:
            case CMD_CALL3:
            case CMD_CALL4:
            case CMD_CALL5:
            case CMD_CALL6:
            case CMD_CALL7:
            case CMD_CALL8: {
                auto r = *(unsigned char*)(bytecode + i++);
                uint64_t n = command - CMD_CALL0;
                printf("call%d R%d", (int) n, (int) r);
                for (int j = 0; j < (int) n; ++j)
                    printf(", R%d", (int) bytecode[i + j]);
                printf("\n");
                i += n;
                break;
            }
// ret
            case CMD_RET: {
                auto has_const = *(unsigned char*)(bytecode + i++);
                uint64_t value = 0;
                if (has_const) {
                    value = *(uint64_t*)(bytecode + i);
                    printf("ret %d\n", (int) value);
                    i += sizeof(uint64_t);
                }
                else {
                    auto r1 = *(unsigned char*)(bytecode + i++);
                    printf("ret R%d\n", (int) r1);
                }
                break;
            }
            case CMD_LEAVE: {
                printf("leave\n");
                break;
            }
            default:
                 //printf("REG[0] = %d\n", (int32_t)REG[0]);
                fprintf(stderr, "wrong command\n");
                exit(1);
        }
    }
}
