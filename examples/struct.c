/*
    This example shows changing structures' fields and passing them to fucnctions work correctly. 
*/


#include <stdio.h>

struct S {
    int a;
    int b;
};

void printf_data(struct S* s) {
    printf("s = {%d, %d}\n", s->a, s->b);
}

int main()
{
    struct S s;
    s.a = 5;
    s.b = 7;
    printf_data(&s);
}
