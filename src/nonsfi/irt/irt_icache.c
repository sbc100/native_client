/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <errno.h>
#include <stddef.h>

#if defined(__arm__) || defined(__mips__)
#include "native_client/src/nonsfi/linux/linux_syscall_wrappers.h"
#include "native_client/src/public/linux_syscalls/sys/syscall.h"
#endif

#if defined(__arm__)
int irt_clear_cache(void *addr, size_t size) {
  /*
   * The third argument of cacheflush is just ignored for now and should
   * always be zero.
   */
  int result = linux_syscall3(__NR_ARM_cacheflush,
                              (uint32_t) addr,
                              (uintptr_t) addr + size,
                              0);
  if (linux_is_error_result(result))
    return -result;
  return 0;
}
#elif defined(__mips__)
int irt_clear_cache(void *addr, size_t size) {
  int result = linux_syscall3(__NR_cacheflush,
                              (uint32_t) addr,
                              (uint32_t) size,
                              3 /* BCACHE */);
  if (linux_is_error_result(result))
    return -result;
  return 0;
}
#endif
