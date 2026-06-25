#ifndef _SWITCH_LINUX_VERSION_H
#define _SWITCH_LINUX_VERSION_H
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + ((c) > 255 ? 255 : (c)))
#define LINUX_VERSION_CODE 0
#endif
