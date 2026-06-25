#include "switch_jitmem.h"

#include <errno.h>
#include <switch.h>

uintptr_t g_switchJitRxBase = 0;
uintptr_t g_switchJitRxEnd = 0;
intptr_t g_switchJitWriteOffset = 0;

static Jit s_jit;
static int s_have = 0;

void *switchExecReserve(size_t bytes) {
  if (s_have) {
    return NULL;
  }
  size_t size = (bytes + 0xFFF) & ~(size_t)0xFFF;
  if (R_FAILED(jitCreate(&s_jit, size))) {
    errno = ENOMEM;
    return NULL;
  }
  void *rx = jitGetRxAddr(&s_jit);
  void *rw = jitGetRwAddr(&s_jit);
  g_switchJitRxBase = (uintptr_t)rx;
  g_switchJitRxEnd = (uintptr_t)rx + size;
  g_switchJitWriteOffset = (intptr_t)((uintptr_t)rw - (uintptr_t)rx);
  s_have = 1;
  return rx;
}

int switchExecProtect(void *addr, size_t bytes, int executable) {
  if (executable && s_have) {
    uintptr_t a = (uintptr_t)addr;
    if (a >= g_switchJitRxBase && a < g_switchJitRxEnd) {
      armDCacheFlush((void *)(a + (uintptr_t)g_switchJitWriteOffset), bytes);
      armICacheInvalidate(addr, bytes);
    }
  }
  return 0;
}

int switchExecDecommit(void *addr, size_t bytes) {
  (void)addr;
  (void)bytes;
  return 0;
}

void switchExecRelease(void *addr, size_t bytes) {
  (void)addr;
  (void)bytes;
  if (s_have) {
    jitClose(&s_jit);
    s_have = 0;
  }
  g_switchJitRxBase = 0;
  g_switchJitRxEnd = 0;
  g_switchJitWriteOffset = 0;
}
