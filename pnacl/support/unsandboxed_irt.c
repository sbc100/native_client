/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"
#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"

/*
 * This is an implementation of NaCl's IRT interfaces that runs
 * outside of the NaCl sandbox.
 *
 * This allows PNaCl to be used as a portability layer without the
 * SFI-based sandboxing.  PNaCl pexes can be translated to
 * non-SFI-sandboxed native code and linked against this IRT
 * implementation.
 */


void _user_start(void *info);

static __thread void *g_tls_value;


/*
 * The IRT functions in irt.h are declared as taking "struct timespec"
 * and "struct timeval" pointers, but these are really "struct
 * nacl_abi_timespec" and "struct nacl_abi_timeval" pointers in this
 * unsandboxed context.
 *
 * To avoid changing irt.h for now and also avoid casting function
 * pointers, we use the same type signatures as in irt.h and do the
 * casting here.
 */
static void convert_from_nacl_timespec(struct timespec *dest,
                                       const struct timespec *src_nacl) {
  const struct nacl_abi_timespec *src =
      (const struct nacl_abi_timespec *) src_nacl;
  dest->tv_sec = src->tv_sec;
  dest->tv_nsec = src->tv_nsec;
}

static void convert_to_nacl_timespec(struct timespec *dest_nacl,
                                     const struct timespec *src) {
  struct nacl_abi_timespec *dest = (struct nacl_abi_timespec *) dest_nacl;
  dest->tv_sec = src->tv_sec;
  dest->tv_nsec = src->tv_nsec;
}

static void convert_to_nacl_timeval(struct timeval *dest_nacl,
                                    const struct timeval *src) {
  struct nacl_abi_timeval *dest = (struct nacl_abi_timeval *) dest_nacl;
  dest->nacl_abi_tv_sec = src->tv_sec;
  dest->nacl_abi_tv_usec = src->tv_usec;
}

static void convert_to_nacl_stat(struct stat *dest_nacl,
                                 const struct stat *src) {
  struct nacl_abi_stat *dest = (struct nacl_abi_stat *) dest_nacl;
  dest->nacl_abi_st_dev = src->st_dev;
  dest->nacl_abi_st_ino = src->st_ino;
  dest->nacl_abi_st_mode = src->st_mode;
  dest->nacl_abi_st_nlink = src->st_nlink;
  dest->nacl_abi_st_uid = src->st_uid;
  dest->nacl_abi_st_gid = src->st_gid;
  dest->nacl_abi_st_rdev = src->st_rdev;
  dest->nacl_abi_st_size = src->st_size;
  dest->nacl_abi_st_blksize = src->st_blksize;
  dest->nacl_abi_st_blocks = src->st_blocks;
  dest->nacl_abi_st_atime = src->st_atim.tv_sec;
  dest->nacl_abi_st_atimensec = src->st_atim.tv_nsec;
  dest->nacl_abi_st_mtime = src->st_mtim.tv_sec;
  dest->nacl_abi_st_mtimensec = src->st_mtim.tv_nsec;
  dest->nacl_abi_st_ctime = src->st_ctim.tv_sec;
  dest->nacl_abi_st_ctimensec = src->st_ctim.tv_nsec;
}

static int check_error(int result) {
  if (result != 0) {
    /*
     * Check that we really have an error and don't indicate success
     * mistakenly.
     */
    assert(errno != 0);
    return errno;
  }
  return 0;
}

static int irt_close(int fd) {
  return check_error(close(fd));
}

static int irt_dup(int fd, int *new_fd) {
  int result = dup(fd);
  if (result < 0)
    return errno;
  *new_fd = result;
  return 0;
}

static int irt_dup2(int fd, int new_fd) {
  int result = dup2(fd, new_fd);
  if (result < 0)
    return errno;
  assert(result == new_fd);
  return 0;
}

static int irt_read(int fd, void *buf, size_t count, size_t *nread) {
  int result = read(fd, buf, count);
  if (result < 0)
    return errno;
  *nread = result;
  return 0;
}

static int irt_write(int fd, const void *buf, size_t count, size_t *nwrote) {
  int result = write(fd, buf, count);
  if (result < 0)
    return errno;
  *nwrote = result;
  return 0;
}

static int irt_seek(int fd, nacl_abi_off_t offset, int whence,
                    nacl_abi_off_t *new_offset) {
  off_t result = lseek(fd, offset, whence);
  if (result < 0)
    return errno;
  *new_offset = result;
  return 0;
}

static int irt_fstat(int fd, struct stat *st) {
  /* TODO(mseaborn): Implement this and convert "struct stat". */
  return ENOSYS;
}

static void irt_exit(int status) {
  _exit(status);
}

static int irt_clock_func(clock_t *ticks) {
  clock_t result = clock();
  if (result == (clock_t) -1)
    return errno;
  *ticks = result;
  return 0;
}

static int irt_gettod(struct timeval *time_nacl) {
  struct timeval time;
  int result = check_error(gettimeofday(&time, NULL));
  convert_to_nacl_timeval(time_nacl, &time);
  return result;
}

static int irt_sched_yield(void) {
  return check_error(sched_yield());
}

static int irt_nanosleep(const struct timespec *requested_nacl,
                         struct timespec *remaining_nacl) {
  struct timespec requested;
  struct timespec remaining;
  convert_from_nacl_timespec(&requested, requested_nacl);
  int result = check_error(nanosleep(&requested, &remaining));
  if (remaining_nacl != NULL)
    convert_to_nacl_timespec(remaining_nacl, &remaining);
  return result;
}

static int irt_sysconf(int name, int *value) {
  switch (name) {
    case NACL_ABI__SC_PAGESIZE:
      /*
       * For now, return the host's page size (typically 4k) rather
       * than 64k (NaCl's usual page size), which pexes will usually
       * be tested with.  We could change this to 64k, but then the
       * mmap() we define here should round up requested sizes to
       * multiples of 64k.
       */
      *value = getpagesize();
      return 0;
    case NACL_ABI__SC_NPROCESSORS_ONLN: {
      int result = sysconf(_SC_NPROCESSORS_ONLN);
      if (result == 0)
        return errno;
      *value = result;
      return 0;
    }
    default:
      return EINVAL;
  }
}

static int irt_mmap(void **addr, size_t len, int prot, int flags,
                    int fd, off_t off) {
  void *result = mmap(*addr, len, prot, flags, fd, off);
  if (result == MAP_FAILED)
    return errno;
  *addr = result;
  return 0;
}

static int irt_munmap(void *addr, size_t len) {
  return check_error(munmap(addr, len));
}

static int tls_init(void *ptr) {
  g_tls_value = ptr;
  return 0;
}

static void *tls_get(void) {
  return g_tls_value;
}

void *__nacl_read_tp(void) {
  return g_tls_value;
}

struct thread_args {
  void (*start_func)(void);
  void *thread_ptr;
};

static void *start_thread(void *arg) {
  struct thread_args args = *(struct thread_args *) arg;
  free(arg);
  g_tls_value = args.thread_ptr;
  args.start_func();
  abort();
}

static int thread_create(void (*start_func)(void), void *stack,
                         void *thread_ptr) {
  /*
   * For now, we ignore the stack that user code provides and just use
   * the stack that the host libpthread allocates.
   */
  pthread_attr_t attr;
  int error = pthread_attr_init(&attr);
  if (error != 0)
    return error;
  error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (error != 0)
    return error;
  struct thread_args *args = malloc(sizeof(struct thread_args));
  if (args == NULL) {
    error = ENOMEM;
    goto cleanup;
  }
  args->start_func = start_func;
  args->thread_ptr = thread_ptr;
  pthread_t tid;
  error = pthread_create(&tid, &attr, start_thread, args);
  if (error != 0)
    free(args);
 cleanup:
  pthread_attr_destroy(&attr);
  return error;
}

static void thread_exit(int32_t *stack_flag) {
  *stack_flag = 0;  /* Indicate that the user code's stack can be freed. */
  pthread_exit(NULL);
}

static int thread_nice(const int nice) {
  return 0;
}

static int futex_wait_abs(volatile int *addr, int value,
                          const struct timespec *abstime_nacl) {
  struct timespec reltime;
  struct timespec *reltime_ptr = NULL;
  if (abstime_nacl != NULL) {
    struct timespec time_now;
    if (clock_gettime(CLOCK_REALTIME, &time_now) != 0)
      return errno;

    /* Convert the absolute time to a relative time. */
    const struct nacl_abi_timespec *abstime =
        (const struct nacl_abi_timespec *) abstime_nacl;
    reltime.tv_sec = abstime->tv_sec - time_now.tv_sec;
    reltime.tv_nsec = abstime->tv_nsec - time_now.tv_nsec;
    if (reltime.tv_nsec < 0) {
      reltime.tv_sec -= 1;
      reltime.tv_nsec += 1000000000;
    }
    /*
     * Linux's FUTEX_WAIT returns EINVAL if given a negative relative
     * time.  But an absolute time that's in the past is a valid
     * argument, for which we need to return ETIMEDOUT instead.
     */
    if (reltime.tv_sec < 0)
      return ETIMEDOUT;
    reltime_ptr = &reltime;
  }
  return check_error(syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, value,
                             reltime_ptr, 0, 0));
}

static int futex_wake(volatile int *addr, int nwake, int *count) {
  int result = syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, nwake, 0, 0, 0);
  if (result < 0)
    return errno;
  *count = result;
  return 0;
}

static int irt_clock_getres(clockid_t clk_id, struct timespec *time_nacl) {
  struct timespec time;
  int result = check_error(clock_getres(clk_id, &time));
  convert_to_nacl_timespec(time_nacl, &time);
  return result;
}

static int irt_clock_gettime(clockid_t clk_id, struct timespec *time_nacl) {
  struct timespec time;
  int result = check_error(clock_gettime(clk_id, &time));
  convert_to_nacl_timespec(time_nacl, &time);
  return result;
}

static int irt_open(const char *pathname, int flags, mode_t mode, int *new_fd) {
  int fd = open(pathname, flags, mode);
  if (fd < 0)
    return errno;
  *new_fd = fd;
  return 0;
}

static int irt_stat(const char *pathname, struct stat *stat_info_nacl) {
  struct stat stat_info;
  if (stat(pathname, &stat_info) != 0)
    return errno;
  convert_to_nacl_stat(stat_info_nacl, &stat_info);
  return 0;
}

static int irt_mkdir(const char *pathname, mode_t mode) {
  return check_error(mkdir(pathname, mode));
}

static int irt_rmdir(const char *pathname) {
  return check_error(rmdir(pathname));
}

static int irt_chdir(const char *pathname) {
  return check_error(chdir(pathname));
}

static int irt_getcwd(char *pathname, size_t len) {
  if (getcwd(pathname, len) == NULL)
    return errno;
  return 0;
}

static int irt_unlink(const char *pathname) {
  return check_error(unlink(pathname));
}

static void irt_stub_func(const char *name) {
  fprintf(stderr, "Error: Unimplemented IRT function: %s\n", name);
  abort();
}

#define DEFINE_STUB(name) \
    static void irt_stub_##name() { irt_stub_func(#name); }
#define USE_STUB(s, name) (typeof(s.name)) irt_stub_##name

static const struct nacl_irt_basic irt_basic = {
  irt_exit,
  irt_gettod,
  irt_clock_func,
  irt_nanosleep,
  irt_sched_yield,
  irt_sysconf,
};

DEFINE_STUB(getdents)
static const struct nacl_irt_fdio irt_fdio = {
  irt_close,
  irt_dup,
  irt_dup2,
  irt_read,
  irt_write,
  (typeof(irt_fdio.seek)) irt_seek,
  irt_fstat,
  USE_STUB(irt_fdio, getdents),
};

DEFINE_STUB(mprotect)
static const struct nacl_irt_memory irt_memory = {
  irt_mmap,
  irt_munmap,
  USE_STUB(irt_memory, mprotect),
};

static const struct nacl_irt_tls irt_tls = {
  tls_init,
  tls_get,
};

const static struct nacl_irt_thread irt_thread = {
  thread_create,
  thread_exit,
  thread_nice,
};

const static struct nacl_irt_futex irt_futex = {
  futex_wait_abs,
  futex_wake,
};

const static struct nacl_irt_clock irt_clock = {
  irt_clock_getres,
  irt_clock_gettime,
};

DEFINE_STUB(truncate)
DEFINE_STUB(lstat)
DEFINE_STUB(link)
DEFINE_STUB(rename)
DEFINE_STUB(symlink)
DEFINE_STUB(chmod)
DEFINE_STUB(access)
DEFINE_STUB(readlink)
DEFINE_STUB(utimes)
const static struct nacl_irt_dev_filename irt_dev_filename = {
  irt_open,
  irt_stat,
  irt_mkdir,
  irt_rmdir,
  irt_chdir,
  irt_getcwd,
  irt_unlink,
  USE_STUB(irt_dev_filename, truncate),
  USE_STUB(irt_dev_filename, lstat),
  USE_STUB(irt_dev_filename, link),
  USE_STUB(irt_dev_filename, rename),
  USE_STUB(irt_dev_filename, symlink),
  USE_STUB(irt_dev_filename, chmod),
  USE_STUB(irt_dev_filename, access),
  USE_STUB(irt_dev_filename, readlink),
  USE_STUB(irt_dev_filename, utimes),
};

struct nacl_interface_table {
  const char *name;
  const void *table;
  size_t size;
};

static const struct nacl_interface_table irt_interfaces[] = {
  { NACL_IRT_BASIC_v0_1, &irt_basic, sizeof(irt_basic) },
  { NACL_IRT_FDIO_v0_1, &irt_fdio, sizeof(irt_fdio) },
  { NACL_IRT_MEMORY_v0_3, &irt_memory, sizeof(irt_memory) },
  { NACL_IRT_TLS_v0_1, &irt_tls, sizeof(irt_tls) },
  { NACL_IRT_THREAD_v0_1, &irt_thread, sizeof(irt_thread) },
  { NACL_IRT_FUTEX_v0_1, &irt_futex, sizeof(irt_futex) },
  { NACL_IRT_CLOCK_v0_1, &irt_clock, sizeof(irt_clock) },
  { NACL_IRT_DEV_FILENAME_v0_3, &irt_dev_filename, sizeof(irt_dev_filename) },
};

static size_t irt_interface_query(const char *interface_ident,
                                  void *table, size_t tablesize) {
  unsigned i;
  for (i = 0; i < NACL_ARRAY_SIZE(irt_interfaces); ++i) {
    if (0 == strcmp(interface_ident, irt_interfaces[i].name)) {
      const size_t size = irt_interfaces[i].size;
      if (size <= tablesize) {
        memcpy(table, irt_interfaces[i].table, size);
        return size;
      }
      break;
    }
  }
  fprintf(stderr, "Warning: unavailable IRT interface queried: %s\n",
          interface_ident);
  return 0;
}

int main(int argc, char **argv, char **environ) {
  /* Find size of environ array. */
  size_t env_count = 0;
  while (environ[env_count] != NULL)
    env_count++;

  size_t count =
      1  /* cleanup_func pointer */
      + 2  /* envc and argc counts */
      + argc + 1  /* argv array, with terminator */
      + env_count + 1  /* environ array, with terminator */
      + 4;  /* auxv: 2 entries, one of them the terminator */
  uintptr_t *data = malloc(count * sizeof(uintptr_t));
  if (data == NULL) {
    fprintf(stderr, "Failed to allocate argv/env/auxv array\n");
    return 1;
  }
  size_t pos = 0;
  data[pos++] = 0;  /* cleanup_func pointer */
  data[pos++] = env_count;
  data[pos++] = argc;
  /* Copy arrays, with terminators. */
  int i;
  for (i = 0; i < argc; i++)
    data[pos++] = (uintptr_t) argv[i];
  data[pos++] = 0;
  for (i = 0; i < env_count; i++)
    data[pos++] = (uintptr_t) environ[i];
  data[pos++] = 0;
  /* auxv[0] */
  data[pos++] = AT_SYSINFO;
  data[pos++] = (uintptr_t) irt_interface_query;
  /* auxv[1] */
  data[pos++] = 0;
  data[pos++] = 0;
  assert(pos == count);

  _user_start(data);
  return 1;
}
