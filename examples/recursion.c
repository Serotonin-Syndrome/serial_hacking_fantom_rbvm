/*
    Fibonacci number caclulator.

    This is a simple example of recursion.
*/


#include <stdio.h>

int fib(int n)
{
    return n < 2 ? 1 : fib(n - 1) + fib(n - 2);
}

int main()
{
    printf("%d\n", fib(10));
}
