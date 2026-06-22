#ifndef _SWITCH_PTHREAD_COMPAT_H
#define _SWITCH_PTHREAD_COMPAT_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr);

#ifdef __cplusplus
}
#endif

#endif
