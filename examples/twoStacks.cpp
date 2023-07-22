#include <stdlib.h>

void *ctor() { return malloc(10); }

void dtor(void *adr) { free(adr); }

void myStack2()
{
  void *x = ctor();
  dtor(x);
}

int main() {

    void *x = ctor();
    dtor(x);
    myStack2();
}