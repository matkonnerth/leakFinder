#define _GNU_SOURCE

#include <dlfcn.h>
#include <execinfo.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <unordered_map>
#include <array>
#include <vector>
#include <atomic>

#ifdef __cplusplus
extern "C" {
#endif

const int BACKTRACE_SIZE = 10;
using BacktraceType=std::array<void*,BACKTRACE_SIZE>;

struct LockBacktrace {
  void *lock;
  int lockCount;
  BacktraceType backtrace;
};

class Mutex final {
public:
  Mutex() { logfd = fileno(stderr); }

  ~Mutex() { dump(); fprintf(stderr, "locks %d\n", locks); }

  void lock(pthread_mutex_t *lock) {

    if (!enabled) {
      return;
    }
    m_lockCount++;
    fprintf(stderr, "#lock: %p\n", lock);

    LockBacktrace bt{};
    bt.lock=lock;
    bt.lockCount=m_lockCount;

    int cnt = backtrace(bt.backtrace.data(), bt.backtrace.max_size());

    m_backtraces.insert_or_assign(lock, bt);
    locks++;
  }

  std::vector<LockBacktrace> dump()
  {
    std::vector<LockBacktrace> traces{};
    for(const auto& entry:m_backtraces)
    {
      traces.push_back(entry.second);
    }
    return traces;
  }

private:
  int logfd{};
  int locks{0};
  bool enabled{false};
  std::atomic<int> m_lockCount{};
  
  static inline thread_local std::unordered_map<void*, LockBacktrace> m_backtraces{};
};

static Mutex MyMutex{};

// https://www.gnu.org/software/libc/manual/html_node/Replacing-malloc.html
static void *(*malloc_impl)(size_t) = nullptr;
static void *(*calloc_impl)(size_t, size_t) = nullptr;
static void (*free_impl)(void *) = nullptr;
static void *(*realloc_impl)(void *, size_t) = nullptr;

static int (*pthread_mutex_lock_impl)(pthread_mutex_t *mutex) = nullptr;
// static int pthread_mutex_trylock(pthread_mutex_t *mutex)=nullptr;
// static int pthread_mutex_unlock(pthread_mutex_t *mutex)=nullptr;

static void init_pthread_mutex_lock(void) {
  pthread_mutex_lock_impl =
      (int (*)(pthread_mutex_t *))dlsym(RTLD_NEXT, "pthread_mutex_lock");
  if (nullptr == pthread_mutex_lock_impl) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
  }
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
  if (pthread_mutex_lock_impl == nullptr) {

    init_pthread_mutex_lock();
  }

  auto i = pthread_mutex_lock_impl(mutex);

  MyMutex.lock(mutex);
  return i;
}

#ifdef __cplusplus
}
#endif
