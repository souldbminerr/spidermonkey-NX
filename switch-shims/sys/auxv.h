#ifndef _SWITCH_SHIM_SYS_AUXV_H
#define _SWITCH_SHIM_SYS_AUXV_H

#include <asm/hwcap.h>

#ifndef AT_HWCAP
#define AT_HWCAP 16
#endif
#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline unsigned long getauxval(unsigned long type) {
  if (type == AT_HWCAP) {

    return HWCAP_FP | HWCAP_ASIMD | HWCAP_AES | HWCAP_PMULL | HWCAP_SHA1 |
           HWCAP_SHA2 | HWCAP_CRC32;
  }
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif
