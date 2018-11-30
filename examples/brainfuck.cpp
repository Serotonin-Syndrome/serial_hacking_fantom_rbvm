/*
    A Brainfuck interpreter.

    More complex example showing loops, conditions and switches work correctly.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
void *
xmalloc(size_t n)
{
    void *p = malloc(n);
    if (n && !p) {
        puts("Out of memory.");
        exit(1);
    }
    return p;
}

static
const char *
jump(const char *p, bool forward)
{
    int balance = 0;
    do {
        switch (*p) {
        case '[': ++balance; break;
        case ']': --balance; break;
        }
        if (forward) {
            ++p;
        } else {
            --p;
        }
    } while (balance != 0);
    return p;
}

int main() {
#ifdef JUDGE
    // prints out "Hello World!"
    const char *prog = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
#else
    char *buf = (char *) xmalloc(1024);
    scanf("%1023s", buf);
    const char *prog = buf;
#endif

    const size_t NMEM = 30000;
    char *p = (char *) xmalloc(NMEM);
    for (size_t i = 0; i < NMEM; ++i) {
        p[i] = 0;
    }

    for (; *prog; ++prog) {
        switch (*prog) {
            case '>':
                ++p;
                break;
            case '<':
                --p;
                break;
            case '+':
                ++*p;
                break;
            case '-':
                --*p;
                break;
            case '.':
                printf("%c", *p);
                break;
            case ',':
                scanf("%c", p);
                break;
            case '[':
                if (!*p) {
                    prog = jump(prog, true);
                    --prog;
                }
                break;
                break;
            case ']':
                prog = jump(prog, false);
                break;
        }
    }
}
