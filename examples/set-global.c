/*
    This example shows setting global variable works correctly.
*/

#include <stdio.h>

int x = 1;

int main()
{
    printf("%d\n", x);
    ++x;
    printf("%d\n", x);
}
