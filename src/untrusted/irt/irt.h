/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_H_

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

struct timeval;
struct timespec;
struct stat;
struct dirent;

struct PP_StartFunctions;
struct PP_ThreadFunctions;

#if __cplusplus
extern "C" {
#endif

/*
 * The only interface exposed directly to user code is a single function
 * of this type.  It is passed via the AT_SYSINFO field of the ELF
 * auxiliary vector on the stack at program startup.  The interfaces
 * below are accessed by calling this function with the appropriate
 * interface identifier.
 *
 * This function returns the number of bytes filled in at TABLE, which
 * is never larger than TABLESIZE.  If the interface identifier is not
 * recognized or TABLESIZE is too small, it returns zero.
 *
 * The interface of the query function avoids passing any data pointers
 * back from the IRT to user code.  Only code pointers are passed back.
 * It is an opaque implementation detail (that may change) whether those
 * point to normal untrusted code in the user address space, or whether
 * they point to special trampoline addresses supplied by trusted code.
 */
typedef size_t (*TYPE_nacl_irt_query)(const char *interface_ident,
                                      void *table, size_t tablesize);

/*
 * All functions in IRT vectors return an int, which is zero for success
 * or a (positive) errno code for errors.  Any values are delivered via
 * result parameters.  The only exceptions exit/thread_exit, which can
 * never return, and tls_get, which can never fail.
 */

#define NACL_IRT_BASIC_v0_1     "nacl-irt-basic-0.1"
struct nacl_irt_basic {
  void (*exit)(int status);
  int (*gettod)(struct timeval *tv);
  int (*clock)(clock_t *ticks);
  int (*nanosleep)(const struct timespec *req, struct timespec *rem);
  int (*sched_yield)(void);
  int (*sysconf)(int name, int *value);
};

#define NACL_IRT_FILE_v0_1      "nacl-irt-file-0.1"
struct nacl_irt_file {
  int (*open)(const char *pathname, int oflag, mode_t cmode, int *newfd);
  int (*close)(int fd);
  int (*read)(int fd, void *buf, size_t count, size_t *nread);
  int (*write)(int fd, const void *buf, size_t count, size_t *nwrote);
  int (*seek)(int fd, off_t offset, int whence, off_t *new_offset);
  int (*dup)(int fd, int *newfd);
  int (*dup2)(int fd, int newfd);
  int (*fstat)(int fd, struct stat *);
  int (*stat)(const char *pathname, struct stat *);
  int (*getdents)(int fd, struct dirent *, size_t count, size_t *nread);
};

#define NACL_IRT_MEMORY_v0_1    "nacl-irt-memory-0.1"
struct nacl_irt_memory {
  int (*sysbrk)(void **newbrk);
  int (*mmap)(void **addr, size_t len, int prot, int flags, int fd, off_t off);
  int (*munmap)(void *addr, size_t len);
};

#define NACL_IRT_DYNCODE_v0_1   "nacl-irt-dyncode-0.1"
struct nacl_irt_dyncode {
  int (*dyncode_create)(void *dest, const void *src, size_t size);
  int (*dyncode_modify)(void *dest, const void *src, size_t size);
  int (*dyncode_delete)(void *dest, size_t size);
};

#define NACL_IRT_THREAD_v0_1   "nacl-irt-thread-0.1"
struct nacl_irt_thread {
  int (*thread_create)(void *start_user_address, void *stack,
                       void *tdb, size_t tdb_size);
  void (*thread_exit)(int32_t *stack_flag);
  int (*thread_nice)(const int nice);
};

#define NACL_IRT_MUTEX_v0_1        "nacl-irt-mutex-0.1"
struct nacl_irt_mutex {
  int (*mutex_create)(int *mutex_handle);
  int (*mutex_destroy)(int mutex_handle);
  int (*mutex_lock)(int mutex_handle);
  int (*mutex_unlock)(int mutex_handle);
  int (*mutex_trylock)(int mutex_handle);
};

#define NACL_IRT_COND_v0_1      "nacl-irt-cond-0.1"
struct nacl_irt_cond {
  int (*cond_create)(int *cond_handle);
  int (*cond_destroy)(int cond_handle);
  int (*cond_signal)(int cond_handle);
  int (*cond_broadcast)(int cond_handle);
  int (*cond_wait)(int cond_handle, int mutex_handle);
  int (*cond_timed_wait_abs)(int cond_handle, int mutex_handle,
                             const struct timespec *abstime);
};

#define NACL_IRT_SEM_v0_1       "nacl-irt-sem-0.1"
struct nacl_irt_sem {
  int (*sem_create)(int *sem_handle, int32_t value);
  int (*sem_destroy)(int sem_handle);
  int (*sem_post)(int sem_handle);
  int (*sem_wait)(int sem_handle);
};

#define NACL_IRT_TLS_v0_1       "nacl-irt-tls-0.1"
struct nacl_irt_tls {
  int (*tls_init)(void *tdb, size_t size);
  void *(*tls_get)(void);
};

#define NACL_IRT_BLOCKHOOK_v0_1 "nacl-irt-blockhook-0.1"
struct nacl_irt_blockhook {
  int (*register_block_hooks)(void (*pre)(void), void (*post)(void));
};

#define NACL_IRT_PPAPIHOOK_v0_1 "nacl-irt-ppapihook-0.1"
struct nacl_irt_ppapihook {
  void (*ppapi_start)(const struct PP_StartFunctions *);
  void (*ppapi_register_thread_creator)(const struct PP_ThreadFunctions *);
};

#if __cplusplus
}
#endif

#endif /* NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_H */
