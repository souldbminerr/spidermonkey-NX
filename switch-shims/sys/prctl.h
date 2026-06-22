#ifndef _SWITCH_SHIM_SYS_PRCTL_H
#define _SWITCH_SHIM_SYS_PRCTL_H

#define PR_SET_VMA 0x53564d41
#define PR_SET_VMA_ANON_NAME 0
#define PR_SET_NAME 15
#define PR_GET_NAME 16

#ifdef __cplusplus
extern "C" {
#endif

static inline int prctl(int option, ...) {
  (void)option;
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif
