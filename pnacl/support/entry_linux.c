/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alloca.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__mips__)
/* See comment for apply_reloactions in crtbegin.c */
static inline void __pnacl_start(uint32_t *info) {
  void (*pnacl_start)(uint32_t*);
  __asm__ (".set push\n"
           ".option pic2\n"
           ".set noreorder\n"
           "bal 0f\n"
           "nop\n"
           "0:"
           ".cpload $31\n"
           "lw %0, %%got(0b)($28)\n"
           "addiu %0, %0, %%lo(0b)\n"
           "subu %0, $31, %0\n"
           "lw $31, %%got(__pnacl_start)($28)\n"
           "addu %0, %0, $31\n"
           ".set pop\n"
           : "=r" (pnacl_start)
           :
           :"$28","$31");
  pnacl_start(info);
}
#else
void __pnacl_start(uint32_t *info);
#endif

static int count_envs(char **envp) {
  int count = 0;
  for (; *envp != NULL; envp++)
    count++;
  return count;
}

void __pnacl_start_linux_c(uint32_t *stack) {
  /*
   * The Linux/ELF startup parameters in |stack| is:
   *      [0]             argc, count of argv[] pointers
   *      [1]             argv[0..argc] pointers, argv[argc] being NULL
   *      [1+argc+1]      envp[0..envc] pointers, envp[envc] being NULL
   *      [1+argc+envc+2] auxv[] pairs
   */
  int argc = stack[0];
  char **argv = (char **) (stack + 1);
  char **envp = argv + argc + 1;  /* + 1 for the NULL terminator */
  int envc = count_envs(envp);

  /* cleanup + envc + argc + argv + NULL + envp + NULL + empty auxv */
  int num_entries = 1 + 1 + 1 + argc + 1 + envc + 1 + 2;
  uint32_t *info = (uint32_t *) alloca(sizeof(uint32_t) * num_entries);
  int i = 0;
  int j;
  info[i++] = 0;  /* cleanup_func pointer */
  info[i++] = envc;  /* envc */
  info[i++] = argc;  /* argc */
  for (j = 0; j < argc; j++)
    info[i++] = (uint32_t) argv[j];
  info[i++] = 0;  /* argv terminator */
  for (j = 0; j < envc; j++)
    info[i++] = (uint32_t) envp[j];
  info[i++] = 0;  /* env terminator */
  info[i++] = 0;  /* empty auxv type (AT_NULL) */
  info[i++] = 0;  /* empty auxv value */

  __pnacl_start(info);
}
