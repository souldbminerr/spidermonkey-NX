#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <switch.h>

extern int __real_pthread_create(pthread_t *, const pthread_attr_t *,
                                 void *(*)(void *), void *);

static atomic_int g_core_rr = 0;

typedef struct {
  void *(*fn)(void *);
  void *arg;
} tramp_t;

static void *affinity_tramp(void *p) {
  tramp_t *t = (tramp_t *)p;
  void *(*fn)(void *) = t->fn;
  void *arg = t->arg;
  free(t);

  u64 coremask = 0;
  if (R_FAILED(svcGetInfo(&coremask, InfoType_CoreMask, CUR_PROCESS_HANDLE, 0)) ||
      coremask == 0) {
    coremask = 0x7; // fallback: cores 0,1,2 (homebrew applet set)
  }
  int ncores = __builtin_popcountll(coremask);
  if (ncores < 1)
    ncores = 1;
  int pick = atomic_fetch_add(&g_core_rr, 1) % ncores;
  int pref = 0;
  for (int c = 0, seen = 0; c < 64; c++) {
    if (coremask & (1ull << c)) {
      if (seen == pick) {
        pref = c;
        break;
      }
      seen++;
    }
  }
  svcSetThreadCoreMask(CUR_THREAD_HANDLE, pref, coremask);

  return fn(arg);
}

int __wrap_pthread_create(pthread_t *th, const pthread_attr_t *attr,
                          void *(*fn)(void *), void *arg) {
  tramp_t *t = (tramp_t *)malloc(sizeof(tramp_t));
  if (!t)
    return __real_pthread_create(th, attr, fn, arg);
  t->fn = fn;
  t->arg = arg;
  int r = __real_pthread_create(th, attr, affinity_tramp, t);
  if (r != 0)
    free(t);
  return r;
}
