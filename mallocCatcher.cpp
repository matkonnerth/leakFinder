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

// https://www.gnu.org/software/libc/manual/html_node/Replacing-malloc.html
static void* (*malloc_impl)(size_t)=nullptr;
static void *(*calloc_impl)(size_t, size_t) = nullptr;
static void (*free_impl)(void*)=nullptr;
static void* (*realloc_impl)(void*, size_t)=nullptr;


static void init_malloc(void)
{
    malloc_impl = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
    if (nullptr == malloc_impl) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    }
}

static void init_calloc(void) {
  calloc_impl = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
  if (nullptr == calloc_impl) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
  }
}

static void init_realloc(void) {
  realloc_impl = (void *(*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
  if (nullptr == realloc_impl) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
  }
}

static void init_free(void)
{
    free_impl = (void (*)(void*))dlsym(RTLD_NEXT, "free");
    if (nullptr == free_impl) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    }
}

void *malloc(size_t size)
{
    if(malloc_impl==nullptr) {
        inAlloc=true;
        init_malloc();
        //lets get backtrace/unwind setup
        const int BUFFER_SIZE = 20;
        void *buffer[BUFFER_SIZE];
        int cnt = backtrace(buffer, BUFFER_SIZE);
        backtrace_symbols_fd(buffer, cnt, 2);
    }

    void* p = malloc_impl(size);
    
    if(!inAlloc)
    {
        inAlloc=true;
        MyHeap.alloc(p);
    }
    inAlloc=false;
    return p;
}

void *calloc(size_t elements, size_t size) {
  if (calloc_impl == nullptr) {
    init_calloc();
  }

  void *p = calloc_impl(elements, size);

  //handle like alloc
  MyHeap.alloc(p);
  return p;
}

void *realloc(void* old, size_t size) {
  if (realloc_impl == nullptr) {
    init_realloc();
  }

  void *newAdr = realloc_impl(old, size);

  MyHeap.realloc(old, newAdr);
  return newAdr;
}

void free(void* adr)
{
    if(free_impl==nullptr) {
        init_free();
    }

    MyHeap.dealloc(adr);
    free_impl(adr);
    return;
}



#ifdef __cplusplus
}
#endif
