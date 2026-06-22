#ifndef _SWITCH_SHIM_DLFCN_H
#define _SWITCH_SHIM_DLFCN_H

#define RTLD_LAZY 0x1
#define RTLD_NOW 0x2
#define RTLD_LOCAL 0x0
#define RTLD_GLOBAL 0x100
#define RTLD_DEFAULT ((void *)0)
#define RTLD_NEXT ((void *)-1)

#ifdef __cplusplus
extern "C" {
#endif

static inline void *dlopen(const char *f, int flag) {
  (void)f;
  (void)flag;
  return (void *)0;
}
static inline int dlclose(void *h) {
  (void)h;
  return 0;
}
static inline void *dlsym(void *h, const char *s) {
  (void)h;
  (void)s;
  return (void *)0;
}
static inline char *dlerror(void) {
  return (char *)"dlfcn not supported on Switch";
}

#ifdef __cplusplus
}
#endif

#endif
