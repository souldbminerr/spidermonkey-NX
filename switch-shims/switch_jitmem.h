#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uintptr_t g_switchJitRxBase;
extern uintptr_t g_switchJitRxEnd;
extern intptr_t g_switchJitWriteOffset;

static inline void* switchJitWritable(void* p) {
  uintptr_t a = (uintptr_t)p;
  if (a >= g_switchJitRxBase && a < g_switchJitRxEnd) {
    a += (uintptr_t)g_switchJitWriteOffset;
  }
  return (void*)a;
}

#ifdef __cplusplus
}
#endif
