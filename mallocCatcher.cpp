#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <execinfo.h>

#ifdef __cplusplus
extern "C" {
#endif



bool inAlloc{false};

class Heap final
{
public:
    Heap()
    {
        //log = fopen("memory.txt", "w+");
        logfd = fileno(stderr);
    }

    ~Heap()
    {
        fprintf(stderr, "allocs %d\n", allocs);
        fprintf(stderr, "deallocs %d\n", deallocs);
    }

    void alloc(void* adr)
    {
        if(!enabled)
        {
            return;
        }

        fprintf(stderr, "#malloc: %p\n", adr);

        const int BUFFER_SIZE=10;
        void* buffer[BUFFER_SIZE];
        int cnt = backtrace(buffer, BUFFER_SIZE);       
        backtrace_symbols_fd(buffer, cnt, logfd);
        cnt=0;
        
        allocs++;
    }

    void dealloc(const void* adr)
    {
        if(!enabled)
        {
            return;
        }
        fprintf(stderr, "free(): %p\n", adr);
        deallocs++;
    }

    void enableTracing()
    {
        enabled=true;
    }

private:
    int logfd{};
    int allocs{0};
    int deallocs{0};
    int cnt{0};
    bool enabled{false};
};

static Heap MyHeap{};



static void* (*real_malloc)(size_t)=NULL;
static void (*real_free)(void*)=NULL;

static void init_malloc(void)
{
    real_malloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
    if (NULL == real_malloc) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    }
}

static void init_free(void)
{
    real_free = (void (*)(void*))dlsym(RTLD_NEXT, "free");
    if (NULL == real_free) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    }
}

void *malloc(size_t size)
{
    if(real_malloc==NULL) {
        init_malloc();
    }    

    void* p = real_malloc(size);
    
    if(!inAlloc)
    {
        inAlloc=true;
        MyHeap.alloc(p);        
    }
    inAlloc=false;

    
    

  
    return p;
}

void free(void* adr)
{
    if(real_free==NULL) {
        init_free();
    }

    MyHeap.dealloc(adr);

    //fprintf(log, "#free: %p\n", adr);

    //fprintf(stderr, "free(): %p\n", adr);
    real_free(adr);
    return;
}



#ifdef __cplusplus
}
#endif
