/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/include/nacl_assert.h"


__thread int32_t g_tbss[100];
__thread int32_t g_tdata = -1;

volatile int32_t g_flag = -1;

extern struct nacl_irt_thread_v0_2 nacl_irt_thread;

void ThreadStart(void) {
 char all_zeroes[sizeof(g_tbss)] = {0};

 ASSERT_EQ(memcmp(g_tbss, all_zeroes, sizeof g_tbss), 0);
 ASSERT_EQ(g_tdata, -1);
 nacl_irt_thread.thread_exit((int32_t*)&g_flag);
}

int main() {
  char stack[1024*16];
  int tls_value = 0;
  char* stack_ptr = stack + sizeof(stack);

  int rc = nacl_irt_thread.thread_create(&ThreadStart, stack_ptr,
                                          &tls_value, NULL);

  ASSERT_EQ(rc, 0);

  while(g_flag)
       sched_yield();


  ASSERT_EQ(tls_value, 0);

  return 0;
}
