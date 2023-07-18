#include <stdlib.h>

void* ctor()
{
    return malloc(10);
}

void dtor(void* adr)
{
    free(adr);
}

int main()
{
    while(1)
    {
        void* x = ctor();
        dtor(x);
    }
}