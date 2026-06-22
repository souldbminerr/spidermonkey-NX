#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <switch.h>


#define MAP_FAILED_PTR ((void *)-1)
#define PAGE_SZ 0x1000UL
static inline size_t page_up(size_t n) {
  return (n + PAGE_SZ - 1) & ~(PAGE_SZ - 1);
}

volatile unsigned long g_mmap_calls = 0, g_mmap_total_kb = 0, g_mmap_max_kb = 0,
                       g_mmap_fail = 0, g_mmap_last_kb = 0;

#define ARENA_ALIGN (2UL * 1024 * 1024)
#define BLOCK_SZ (32UL * 1024 * 1024)
#define MAX_FREE 8192
typedef struct {
  uintptr_t start;
  size_t size;
} Region;
static Region s_free[MAX_FREE];
static int s_nfree = 0;
static Mutex s_lock;

static int arena_grow(size_t need) {
  size_t bs = need > BLOCK_SZ ? need : BLOCK_SZ;
  bs = (bs + ARENA_ALIGN - 1) & ~(ARENA_ALIGN - 1);
  if (s_nfree >= MAX_FREE)
    return 0;
  void *p = memalign(ARENA_ALIGN, bs);
  if (!p)
    return 0;
  s_free[s_nfree].start = (uintptr_t)p;
  s_free[s_nfree].size = bs;
  s_nfree++;
  return 1;
}

static void coalesce(void) {
  for (int i = 0; i < s_nfree; i++) {
    for (int j = i + 1; j < s_nfree; j++) {
      if (s_free[i].start + s_free[i].size == s_free[j].start) {
        s_free[i].size += s_free[j].size;
        s_free[j] = s_free[--s_nfree];
        j = i;
      } else if (s_free[j].start + s_free[j].size == s_free[i].start) {
        s_free[j].size += s_free[i].size;
        s_free[i] = s_free[--s_nfree];
        i = -1;
        break;
      }
    }
  }
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, long off) {
  (void)addr;
  (void)prot;
  (void)flags;
  (void)fd;
  (void)off;
  if (!len) {
    errno = EINVAL;
    return MAP_FAILED_PTR;
  }
  size_t size = page_up(len);
  g_mmap_calls++;
  g_mmap_last_kb = (unsigned long)(size >> 10);
  g_mmap_total_kb += (unsigned long)(size >> 10);
  if ((size >> 10) > g_mmap_max_kb)
    g_mmap_max_kb = (unsigned long)(size >> 10);

  void *result = MAP_FAILED_PTR;
  mutexLock(&s_lock);
  for (int attempt = 0; attempt < 2 && result == MAP_FAILED_PTR; attempt++) {
    for (int i = 0; i < s_nfree; i++) {
      if (s_free[i].size >= size) {
        result = (void *)s_free[i].start;
        s_free[i].start += size;
        s_free[i].size -= size;
        if (s_free[i].size == 0)
          s_free[i] = s_free[--s_nfree];
        break;
      }
    }
    if (result == MAP_FAILED_PTR && !arena_grow(size))
      break;
  }
  mutexUnlock(&s_lock);

  if (result == MAP_FAILED_PTR) {
    g_mmap_fail++;
    errno = ENOMEM;
    return MAP_FAILED_PTR;
  }
  memset(result, 0, size);
  return result;
}

void *mmap64(void *a, size_t l, int p, int f, int fd, long o) {
  return mmap(a, l, p, f, fd, o);
}

int munmap(void *addr, size_t len) {
  if (!addr || addr == MAP_FAILED_PTR || !len)
    return 0;
  size_t size = page_up(len);
  mutexLock(&s_lock);
  if (s_nfree < MAX_FREE) {
    s_free[s_nfree].start = (uintptr_t)addr;
    s_free[s_nfree].size = size;
    s_nfree++;
    coalesce();
  }
  mutexUnlock(&s_lock);
  return 0;
}

int mprotect(void *a, size_t l, int prot) {
  (void)a;
  (void)l;
  (void)prot;
  return 0;
}
int madvise(void *a, size_t l, int advice) {
  (void)a;
  (void)l;
  (void)advice;
  return 0;
}

void *mremap(void *old, size_t os, size_t ns, int flags, ...) {
  (void)old;
  (void)os;
  (void)ns;
  (void)flags;
  errno = ENOMEM;
  return MAP_FAILED_PTR;
}

int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr) {
  (void)thread;
  if (!attr)
    return EINVAL;
  pthread_attr_init(attr);
  MemoryInfo mi = {0};
  u32 pageinfo = 0;
  uintptr_t here = (uintptr_t)__builtin_frame_address(0);
  if (R_SUCCEEDED(svcQueryMemory(&mi, &pageinfo, here)) && mi.size) {
    pthread_attr_setstack(attr, (void *)(uintptr_t)mi.addr, (size_t)mi.size);
  }
  return 0;
}
