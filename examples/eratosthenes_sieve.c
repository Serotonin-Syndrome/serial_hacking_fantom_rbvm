/*
    Eratosthenes sieve -- method of finding the prime numbers.
    
    This example shows loops and conditions work correctly.
*/


#include <stdio.h>
#include <stdlib.h>
typedef unsigned long UL;

int main() {
#ifdef JUDGE
    UL n = 10000;
#else
    UL n;
    scanf("%lu", &n);
#endif

    char *p = malloc(n + 1);
    if (!p) {
        puts("Out of memory.");
        return 1;
    }
    p[0] = p[1] = 0;
    for (UL i = 2; i <= n; ++i) {
        p[i] = 1;
    }
    for (UL i = 2; i <= n; ++i) {
        if (p[i]) {
            if (i * i <= n) {
                for (UL j = i * i; j <= n; j += i) {
                    p[j] = 0;
                }
            }
            printf("%lu\n", i);
        }
    }
}
