
/* compile with -std=c++17 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <tuple>
#include <map>
#include <vector>
#include <string>
#include <math.h>
#include <assert.h>
#include <array>

#include "opcode.h"
#include "reader.h"

#define PAIR(S_) (int) (S_).size(), (S_).data()


struct FunctionHeader
{
    unsigned char native;
};

struct Function
{
    FunctionHeader header;

    uint64_t offset, nargs, nskip;

    Function(uint64_t offset, uint64_t nargs, uint64_t nskip)
        : header{0}, offset(offset), nargs(nargs), nskip(nskip) {}
};

struct NativeFunction
{
    typedef uint64_t (*Call)(unsigned nargs, const unsigned char *regs);
    FunctionHeader header;
    Call ptr;

    NativeFunction(Call ptr_) : header{1}, ptr(ptr_) {}
};


typedef std::array<uint64_t, 256> stack_frame;
static std::vector<stack_frame> reg_stack;

#define REG reg_stack.back().data()

static std::vector<std::pair<unsigned, unsigned char>> call_stack;

static std::map<std::string, void*> names;



static void init_call(unsigned n, const char* bytecode, unsigned& i) {
    std::vector<uint64_t> args(n);
    for (auto& arg : args)
        arg = REG[*(unsigned char*)(bytecode + i++)];

    reg_stack.emplace_back();

    for (unsigned j = 0; j < n; ++j)
        REG[j + 1] = args[j];
}


template <typename T, class Instruction>
void perform_reg_val_instruction(const char* bytecode, unsigned& i, Instruction instruction) {
    auto has_const = *(unsigned char*)(bytecode + i++);
    auto r1 = *(unsigned char*)(bytecode + i++);


    if (has_const) {
        auto value = *(T*)(bytecode + i);
#ifdef TEXT
        printf("~~~ R%d, %d\n", (int) r1, (int) value);
#endif
        i += sizeof(T);
        REG[r1] = (uint64_t)(instruction(*(T*)(REG + r1), value));
    }
    else {
        auto r2 = *(unsigned char*)(bytecode + i++);
#ifdef TEXT
        printf("~~~ R%d, R%d\n", (int) r1, (int) r2);
#endif
        REG[r1] = (uint64_t)(instruction(*(T*)(REG + r1), *(T*)(REG + r2)));
    }
}

template<typename T>
static void ld(const char *bytecode, unsigned &i)
{
    auto has_const = *(unsigned char*)(bytecode + i++);
    auto r1 = *(unsigned char*)(bytecode + i++);

    if (has_const) {
        auto ptr = *(T**)(bytecode + i);
#ifdef TEXT
        printf("ld%d R%d, %p\n", (int) sizeof(T) * 8, (int) r1, ptr);
#endif
        i += sizeof(T*);

        REG[r1] = *ptr;
    } else {
        auto r2 = *(unsigned char*)(bytecode + i++);
#ifdef TEXT
        printf("ld%d R%d, R%d\n", (int) sizeof(T) * 8, (int) r1, (int) r2);
#endif
        REG[r1] = *(T*) REG[r2];
    }
}

template <typename T>
void st(const char* bytecode, unsigned& i) {
    auto has_const = *(unsigned char*)(bytecode + i++);

    if (has_const) {
        auto value_ptr = *(T**)(bytecode + i);
        i += sizeof(T*);

        auto r2 = *(unsigned char*)(bytecode + i++);
#ifdef TEXT
        printf("st%d %p, R%d\n", (int) sizeof(T) * 8, (void *) value_ptr, (int) r2);
#endif
        *value_ptr = REG[r2];
    }
    else {
        auto r1 = *(unsigned char*)(bytecode + i++);
        auto r2 = *(unsigned char*)(bytecode + i++);
#ifdef TEXT
        printf("st%d R%d, R%d\n", (int) sizeof(T) * 8, (int) r1, (int) r2);
#endif

        *(T*)REG[r1] = REG[r2];
    }
}

static void * copy_static_str(std::pair<const char *, size_t> span) {
    char *ptr = new char[span.second];
    if (span.second) {
        memcpy(ptr, span.first, span.second);
    }
    return ptr;
}

static void register_globals() {
    names["puts"] = new NativeFunction([](unsigned nargs, const unsigned char *regs) -> uint64_t {
        if (nargs != 1) {
            fprintf(stderr, "'puts' requires exactly 1 argument\n");
            exit(1);
        }
        auto r = (unsigned char) regs[0];
        void *ptr = (void *) REG[r];
        puts((const char *) ptr);
        return 0;
    });

    names["printf"] = new NativeFunction([](unsigned nargs, const unsigned char *regs) -> uint64_t {
        if (!nargs) {
            fprintf(stderr, "'printf' requires at least 1 argument\n");
            exit(1);
        }
        const char *fmt = (const char* ) REG[regs[0]];
        switch (nargs) {
        case 1: return printf(fmt);
        case 2: return printf(fmt, REG[regs[1]]);
        case 3: return printf(fmt, REG[regs[1]], REG[regs[2]]);
        case 4: return printf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]]);
        case 5: return printf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]], REG[regs[4]]);
        case 6: return printf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]], REG[regs[4]], REG[regs[5]]);
        case 7: return printf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]], REG[regs[4]], REG[regs[5]], REG[regs[6]]);
        case 8: return printf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]], REG[regs[4]], REG[regs[5]], REG[regs[6]], REG[regs[7]]);
        default: assert(0);
        }
    });

    names["scanf"] = names["__isoc99_scanf"] = new NativeFunction([](unsigned nargs, const unsigned char *regs) -> uint64_t {
        if (!nargs) {
            fprintf(stderr, "'scanf' requires at least 1 argument\n");
            exit(1);
        }
        const char *fmt = (const char* ) REG[regs[0]];
        switch (nargs) {
        case 1: return scanf(fmt);
        case 2: return scanf(fmt, REG[regs[1]]);
        case 3: return scanf(fmt, REG[regs[1]], REG[regs[2]]);
        case 4: return scanf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]]);
        case 5: return scanf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]], REG[regs[4]]);
        case 6: return scanf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]], REG[regs[4]], REG[regs[5]]);
        case 7: return scanf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]], REG[regs[4]], REG[regs[5]], REG[regs[6]]);
        case 8: return scanf(fmt, REG[regs[1]], REG[regs[2]], REG[regs[3]], REG[regs[4]], REG[regs[5]], REG[regs[6]], REG[regs[7]]);
        default: assert(0);
        }
    });

    names["exit"] = new NativeFunction([](unsigned nargs, const unsigned char *regs) -> uint64_t {
        if (nargs != 1) {
            fprintf(stderr, "'exit' requires exactly 1 argument\n");
            exit(1);
        }
        exit(REG[regs[0]]);
    });

    names["malloc"] = new NativeFunction([](unsigned nargs, const unsigned char *regs) -> uint64_t {
        if (nargs != 1) {
            fprintf(stderr, "'malloc' requires exactly 1 argument\n");
            exit(1);
        }
        return (uintptr_t) ::malloc(REG[regs[0]]);
    });

    names["free"] = new NativeFunction([](unsigned nargs, const unsigned char *regs) -> uint64_t {
        if (nargs != 1) {
            fprintf(stderr, "'free' requires exactly 1 argument\n");
            exit(1);
        }
        free((void*)REG[regs[0]]);
        return 0;
    });
}

int main(int argc, char** argv) {
    register_globals();
    reg_stack.emplace_back();

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
        auto command = *(unsigned char*)(bytecode + i);
#ifdef TEXT
        printf("# offset %u: %d\n", i, (int) command);
#endif
        ++i;

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

                // function's body beginning
#ifdef TEXT
                printf("fd '%.*s', nargs=%d, nskip=%d\n", PAIR(name), (int) nargs, (int) nskip);
#endif
                names[name] = new Function(i, nargs, nskip);
                // skip it
                i += nskip;

                break;
            }
// moving
            case CMD_MOV: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                      {(void) a; return b;});
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

#ifdef TEXT
                printf("gg '%.*s', R%d\n", PAIR(name), (int) r);
#endif
                REG[r] = (uintptr_t) names[name];
                break;
            }
            case CMD_SG: {
                auto len = *(uint32_t*)(bytecode + i);
                i += sizeof(uint32_t);

                std::string name(bytecode + i, bytecode + i + len);
                i += len;

                auto r = *(unsigned char*)(bytecode + i);
                ++i;
#ifdef TEXT
                printf("sg '%.*s', R%d\n", PAIR(name), (int) r);
#endif
                //*(uint64_t*)(global_pool + top) = REG[r];
                names[name] = (void *) REG[r];
                //top += sizeof(uint64_t);

                break;
            }
            case CMD_CSS: {
                // reading <str>
                auto len = *(uint32_t*)(bytecode + i);
                i += sizeof(uint32_t);

                std::pair<const char *, size_t> span = {bytecode + i, len};
                i += len;

                auto r = *(unsigned char*)(bytecode + i);
                ++i;

#ifdef TEXT
                printf("css '%.*s', R%d\n", (int) span.second, span.first, (int) r);
#endif
                REG[r] = (uintptr_t) copy_static_str(span);
//endif
                break;
            }
            case CMD_CSS_DYN: {
                auto r1 = *(unsigned char*)(bytecode + i++);
                auto r2 = *(unsigned char*)(bytecode + i++);
                char* str = new char[8];
                memcpy(str, &REG[r2], 8);
                REG[r1] = (uintptr_t)(str);
                break;
            }
// ptrs
            case CMD_LD8:  ld<uint8_t>(bytecode, i); break;
            case CMD_LD16: ld<uint16_t>(bytecode, i); break;
            case CMD_LD32: ld<uint32_t>(bytecode, i); break;
            case CMD_LD64: ld<uint64_t>(bytecode, i); break;

            // any other solutions?
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
#ifdef TEXT
                printf("lea R%d, R%d\n", (int) r1, (int) r2);
#endif
                REG[r1] = (uint64_t)(REG + r2);
//endif
                break;
            }
// integer arithmetic
            case CMD_IADD: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a + b;});
                break;
            }
            case CMD_ISUB: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a - b;});
                break;
            }
            case CMD_SMUL: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a * b;});
                break;
            }
            case CMD_UMUL: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a * b;});
                break;
            }
            case CMD_SREM: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a % b;});
                break;
            }
            case CMD_UREM: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a % b;});
                break;
            }
            case CMD_SDIV: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a / b;});
                break;
            }
            case CMD_UDIV: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a / b;});
                break;
            }
// bitwise
            case CMD_AND: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a & b;});
                break;
            }
            case CMD_OR: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a | b;});
                break;
            }
            case CMD_XOR: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a ^ b;});
                break;
            }
            case CMD_SHL: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a << b;});
                break;
            }
            // check
            case CMD_LSHR: {
                perform_reg_val_instruction<uint32_t>(bytecode, i, [](uint32_t a, uint32_t b)
                                                                    {return a >> b;});
                break;
            }
            case CMD_ASHR: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                    {return a >> b;});
                break;
            }
            case CMD_INEG: {
                auto r = *(unsigned char*)(bytecode + i++);
#ifdef TEXT
                printf("ineg R%d\n", (int) r);
#endif
                REG[r] = ~REG[r];
                break;
            }
// float-point arithmetic
            case CMD_FADD: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a + b;});
                break;
            }
            case CMD_FSUB: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a - b;});
                break;
            }
            case CMD_FMUL: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a * b;});
                break;
            }
            case CMD_FDIV: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a / b;});
                break;
            }
            case CMD_FREM: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return fmod(a, b);});
                break;
            }
// bitwise comparison
            case CMD_EQ: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                      {return a == b;});
                break;
            }
            case CMD_NE: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                      {return a != b;});
                break;
            }
// integer comparison
            case CMD_SLT: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](int64_t a, int64_t b)
                                                                      {return a < b;});
                break;
            }
            case CMD_SLE: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](int64_t a, int64_t b)
                                                                      {return a <= b;});
                break;
            }
            case CMD_SGT: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](int64_t a, int64_t b)
                                                                      {return a > b;});
                break;
            }
            case CMD_SGE: {
                perform_reg_val_instruction<int64_t>(bytecode, i, [](int64_t a, int64_t b)
                                                                      {return a >= b;});
                break;
            }
            case CMD_ULT: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                      {return a < b;});
                break;
            }
            case CMD_ULE: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                      {return a <= b;});
                break;
            }
            case CMD_UGT: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                      {return a > b;});
                break;
            }
            case CMD_UGE: {
                perform_reg_val_instruction<uint64_t>(bytecode, i, [](uint64_t a, uint64_t b)
                                                                      {return a >= b;});
                break;
            }
// float-point comparison
            case CMD_FEQ: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a == b;});
                break;
            }
            case CMD_FNE: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a != b;});
                break;
            }
            case CMD_FLT: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a < b;});
                break;
            }
            case CMD_FLE: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a <= b;});
                break;
            }
            case CMD_FGT: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a > b;});
                break;
            }
            case CMD_FGE: {
                perform_reg_val_instruction<double>(bytecode, i, [](double a, double b)
                                                                    {return a >= b;});
                break;
            }
// jumps
            case CMD_JMP: {
                auto offset = *(int64_t*)(bytecode + i);
#ifdef TEXT
                printf("jmp %d\n", (int) offset);
#endif
                i += offset - 1;
//endif
                break;
            }
            case CMD_JNZ: {
                auto r = *(unsigned char*)(bytecode + i++);
                auto offset = *(int64_t*)(bytecode + i);
                if (REG[r])
                    i += offset - 2;
                else
                    i += sizeof(int64_t);
                break;
            }
            case CMD_JZ: {
                auto r = *(unsigned char*)(bytecode + i++);
                auto offset = *(int64_t*)(bytecode + i);
#ifdef TEXT
                printf("jz R%d, %d\n", (int) r, (int) offset);
#endif
                if (!REG[r])
                    i += offset - 2;
                else
                    i += sizeof(int64_t);
//endif
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
#ifdef TEXT
                printf("call%d %d", (int) n, (int) r);
                for (int j = 0; j < (int) n; ++j) {
                    printf(", %d", (int) bytecode[i + j]);
                }
                printf("\n");
#endif
                if (REG[r]) {
                    FunctionHeader hdr = *(FunctionHeader*)REG[r];
                    if (hdr.native) {
                        NativeFunction f = *(NativeFunction*)REG[r];
                        REG[r] = f.ptr(n, (unsigned char *) bytecode + i);
                        i += n;
                    } else {
                        Function f = *(Function*)REG[r];

                        assert(n == f.nargs);

                        init_call(n, bytecode, i);

                        call_stack.push_back({i, r});
                        i = f.offset;
                    }
                } else {
                    fprintf(stderr, "(refusing to call a null pointer)\n");
                    i += n;
                }
//endif

                break;
            }
// ret
            case CMD_RET: {
                auto has_const = *(unsigned char*)(bytecode + i++);
                uint64_t value = 0;
                if (has_const) {
                    value = *(uint64_t*)(bytecode + i);
#ifdef TEXT
                    printf("ret %d\n", (int) value);
#endif
                    i += sizeof(uint64_t);
                }
                else {
                    auto r1 = *(unsigned char*)(bytecode + i++);
#ifdef TEXT
                    printf("ret R%d\n", (int) r1);
#endif
                    value = REG[r1];
                }

                unsigned char r = 0;
                std::tie(i, r) = call_stack.back();
                reg_stack.pop_back();

                REG[r] = value;

                call_stack.pop_back();
                break;
            }
            case CMD_LEAVE: {
#ifdef TEXT
                printf("leave\n");
#endif
                unsigned char r = 0;
                std::tie(i, r) = call_stack.back();

                reg_stack.pop_back();

                call_stack.pop_back();
                break;
            }
            default:
                 //printf("REG[0] = %d\n", (int32_t)REG[0]);
                fprintf(stderr, "wrong command\n");
                exit(1);
        }
    }
}
