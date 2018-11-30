/*
    This example shows classes and inheritance work correctly.
*/


#include <stdio.h>


struct Animal
{
    virtual void voice() = 0;
};


struct Snake : public Animal
{
    virtual void voice() override
    {
        puts("HSSSssssssssss......");
    }
};


int
main()
{
    Snake Drake;
    Drake.voice();
}
