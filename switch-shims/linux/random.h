#ifndef _SWITCH_SHIM_LINUX_RANDOM_H
#define _SWITCH_SHIM_LINUX_RANDOM_H

#include <stddef.h>
#include <sys/types.h>

#define GRND_NONBLOCK 0x0001
#define GRND_RANDOM 0x0002
#define GRND_INSECURE 0x0004

#ifdef __cplusplus
extern "C" {
#endif

extern void randomGet(void *buf, size_t len);

static inline ssize_t getrandom(void *buf, size_t buflen, unsigned int flags) {
  (void)flags;
  randomGet(buf, buflen);
  return (ssize_t)buflen;
}

#ifdef __cplusplus
}
#endif

#endif
