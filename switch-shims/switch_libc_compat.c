#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern void randomGet(void *buf, size_t len);
extern unsigned int threadGetCurHandle(void);

int *__errno_location(void) { return &errno; }

off_t lseek64(int fd, off_t offset, int whence) {
  return lseek(fd, offset, whence);
}
int fstat64(int fd, struct stat *st) { return fstat(fd, st); }
int stat64(const char *path, struct stat *st) { return stat(path, st); }
int open64(const char *path, int flags, ...) {
  va_list ap;
  va_start(ap, flags);
  int mode = va_arg(ap, int);
  va_end(ap);
  return open(path, flags, mode);
}

extern void *memalign(size_t alignment, size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size) {
  void *p = memalign(alignment, size);
  if (!p)
    return 12;
  *memptr = p;
  return 0;
}

long sysconf(int name) {
  switch (name) {
  case 8:
    return 0x1000;
  case 0x4f:
    return 1;
  default:
    return -1;
  }
}

unsigned long getauxval(unsigned long type) {
  if (type == 16)
    return (1u << 0) | (1u << 1) | (1u << 3) | (1u << 4) | (1u << 5) |
           (1u << 6) | (1u << 7);
  return 0;
}

int getrusage(int who, void *usage) {
  (void)who;
  (void)usage;
  return 0;
}

int dl_iterate_phdr(void *cb, void *data) {
  (void)cb;
  (void)data;
  return 0;
}

int pthread_setname_np(unsigned long thread, const char *name) {
  (void)thread;
  (void)name;
  return 0;
}

long syscall(long number, ...) {
  va_list ap;
  va_start(ap, number);
  long r;
  if (number == 178) {
    r = (long)threadGetCurHandle();
  } else if (number == 278) {
    void *b = va_arg(ap, void *);
    size_t l = va_arg(ap, size_t);
    randomGet(b, l);
    r = (long)l;
  } else {
    errno = 38;
    r = -1;
  }
  va_end(ap);
  return r;
}
