#include <stdlib.h>

void *ctor() { return malloc(10); }

void dtor(void *adr) { free(adr); }

int main() {

    void *x = ctor();
    x = realloc(x, 10000);
    dtor(x);
}