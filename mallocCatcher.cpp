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
        logfd = fileno(stderr);
    }

    ~Heap()
    {
        fprintf(stderr, "allocs %d\n", allocs);
        fprintf(stderr, "deallocs %d\n", deallocs);
        fprintf(stderr, "reallocs %d\n", reallocs);
    }

    void alloc(void* adr)
    {
        if(!enabled)
        {
            return;
        }

        fprintf(stderr, "#malloc: %p\n", adr);

        
        void* buffer[BUFFER_SIZE];
        int cnt = backtrace(buffer, BUFFER_SIZE);
        backtrace_symbols_fd(buffer, cnt, logfd);
        cnt=0;
        
        allocs++;
    }

    void realloc(void *old, void* newAdr) {
      if (!enabled) {
        return;
      }

      fprintf(stderr, "#realloc: %p, %p\n", old, newAdr);
      fprintf(stderr, "#free(): %p\n", old);
      fprintf(stderr, "#malloc: %p\n", newAdr);

      void *buffer[BUFFER_SIZE];
      int cnt = backtrace(buffer, BUFFER_SIZE);
      backtrace_symbols_fd(buffer, cnt, logfd);
      cnt = 0;

      reallocs++;
    }

    void dealloc(const void* adr)
    {
        if(!enabled)
        {
            return;
        }
        fprintf(stderr, "#free(): %p\n", adr);
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
    int reallocs{0};
    int cnt{0};
    bool enabled{true};
    const int BUFFER_SIZE = 10;
};

static Heap MyHeap{};



static void* (*real_malloc)(size_t)=nullptr;
static void (*real_free)(void*)=nullptr;
static void* (*real_realloc)(void*, size_t)=nullptr;


static void init_malloc(void)
{
    real_malloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
    if (NULL == real_malloc) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    }
}

static void init_realloc(void) {
  real_realloc = (void *(*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
  if (NULL == real_realloc) {
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
        inAlloc=true;
        init_malloc();
        //lets get backtrace/unwind setup
        const int BUFFER_SIZE = 20;
        void *buffer[BUFFER_SIZE];
        int cnt = backtrace(buffer, BUFFER_SIZE);
        backtrace_symbols_fd(buffer, cnt, 2);
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

void *realloc(void* old, size_t size) {
  if (real_realloc == NULL) {
    init_realloc();
  }

  void *newAdr = real_realloc(old, size);

  MyHeap.realloc(old, newAdr);
  return newAdr;
}

void free(void* adr)
{
    if(real_free==NULL) {
        init_free();
    }

    MyHeap.dealloc(adr);
    real_free(adr);
    return;
}



#ifdef __cplusplus
}
#endif
