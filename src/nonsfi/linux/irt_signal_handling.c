/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/include/elf_constants.h"
#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/nonsfi/linux/irt_signal_handling.h"
#include "native_client/src/nonsfi/linux/linux_sys_private.h"
#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"
#include "native_client/src/nonsfi/linux/linux_syscall_structs.h"
#include "native_client/src/public/linux_syscalls/sys/syscall.h"
#include "native_client/src/public/nonsfi/irt_signal_handling.h"
#include "native_client/src/untrusted/irt/irt.h"

/* Note that MIPS kernel defines different layout with swapped flags and size */
typedef struct compat_sigaltstack {
  uint32_t ss_sp;
  int32_t ss_flags;
  uint32_t ss_size;
} linux_stack_t;

#if defined(__i386__)

/* From linux/arch/x86/include/uapi/asm/sigcontext32.h */
struct sigcontext_ia32 {
  uint16_t gs, __gsh;
  uint16_t fs, __fsh;
  uint16_t es, __esh;
  uint16_t ds, __dsh;
  uint32_t di;
  uint32_t si;
  uint32_t bp;
  uint32_t sp;
  uint32_t bx;
  uint32_t dx;
  uint32_t cx;
  uint32_t ax;
  uint32_t trapno;
  uint32_t err;
  uint32_t ip;
  uint16_t cs, __csh;
  uint32_t flags;
  uint32_t sp_at_signal;
  uint16_t ss, __ssh;
  uint32_t fpstate;
  uint32_t oldmask;
  uint32_t cr2;
};

typedef struct sigcontext_ia32 linux_mcontext_t;

#elif defined(__arm__)

/* From linux/arch/arm/include/uapi/asm/sigcontext.h */
struct sigcontext_arm {
  uint32_t trap_no;
  uint32_t error_code;
  uint32_t oldmask;
  uint32_t arm_r0;
  uint32_t arm_r1;
  uint32_t arm_r2;
  uint32_t arm_r3;
  uint32_t arm_r4;
  uint32_t arm_r5;
  uint32_t arm_r6;
  uint32_t arm_r7;
  uint32_t arm_r8;
  uint32_t arm_r9;
  uint32_t arm_r10;
  uint32_t arm_r11;  /* fp */
  uint32_t arm_r12;  /* ip */
  uint32_t arm_sp;
  uint32_t arm_lr;
  uint32_t arm_pc;
  uint32_t arm_cpsr;
  uint32_t fault_address;
};

typedef struct sigcontext_arm linux_mcontext_t;

#elif defined(__mips__)

/* From linux/arch/mips/include/uapi/asm/sigcontext.h */
struct sigcontext_mips {
   uint32_t sc_regmask;     /* Unused */
   uint32_t sc_status;      /* Unused */
   uint64_t sc_pc;
   uint64_t sc_regs[32];
   uint64_t sc_fpregs[32];
   uint32_t sc_acx;         /* Was sc_ownedfp */
   uint32_t sc_fpc_csr;
   uint32_t sc_fpc_eir;     /* Unused */
   uint32_t sc_used_math;
   uint32_t sc_dsp;         /* dsp status, was sc_ssflags */
   uint64_t sc_mdhi;
   uint64_t sc_mdlo;
   uint32_t sc_hi1;         /* Was sc_cause */
   uint32_t sc_lo1;         /* Was sc_badvaddr */
   uint32_t sc_hi2;         /* Was sc_sigset[4] */
   uint32_t sc_lo2;
   uint32_t sc_hi3;
   uint32_t sc_lo3;
};

typedef struct sigcontext_mips linux_mcontext_t;

#else
#error "unsupported architecture"
#endif

/* From linux/arch/arm/include/asm/ucontext.h */
struct linux_ucontext_t {
  uint32_t uc_flags;
  uint32_t uc_link;
  linux_stack_t uc_stack;
  linux_mcontext_t uc_mcontext;
  linux_sigset_t uc_sigmask;
  /* More data follows which we don't care about. */
};

/*
 * Crash signals to handle.  The differences from SFI NaCl are that
 * NonSFI NaCl does not use NACL_THREAD_SUSPEND_SIGNAL (==SIGUSR1),
 */
static const int kSignals[] = {
#if !defined(__mips__)
  LINUX_SIGSTKFLT,
#endif
  LINUX_SIGINT, LINUX_SIGQUIT, LINUX_SIGILL, LINUX_SIGTRAP, LINUX_SIGBUS,
  LINUX_SIGFPE, LINUX_SIGSEGV,
  /* Handle SIGABRT in case someone sends it asynchronously using kill(). */
  LINUX_SIGABRT,
};

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static NaClExceptionHandler g_signal_handler_function_pointer = NULL;
static NaClExceptionHandler g_exception_handler_function_pointer = NULL;
static int g_signal_handler_initialized = 0;
static int g_tgid = 0;
static int g_main_tid;

#if defined(__mips__)
extern char g_restore_gp2_pc_start[] __attribute__((weak));
extern char g_restore_gp2_pc_end[] __attribute__((weak));
#endif

struct NonSfiExceptionFrame {
  struct NaClExceptionContext context;
  struct NaClExceptionPortableContext portable;
};

static void machine_context_to_register(const linux_mcontext_t *mctx,
                                        NaClUserRegisterState *dest) {
#if defined(__i386__)
#define COPY_REG(A) dest->e##A = mctx->A
  COPY_REG(ax);
  COPY_REG(cx);
  COPY_REG(dx);
  COPY_REG(bx);
  COPY_REG(bp);
  COPY_REG(si);
  COPY_REG(di);
#undef COPY_REG
  dest->stack_ptr = mctx->sp;
  dest->prog_ctr = mctx->ip;
  dest->flags = mctx->flags;
#elif defined(__arm__)
#define COPY_REG(A) dest->A = mctx->arm_##A
  COPY_REG(r0);
  COPY_REG(r1);
  COPY_REG(r2);
  COPY_REG(r3);
  COPY_REG(r4);
  COPY_REG(r5);
  COPY_REG(r6);
  COPY_REG(r7);
  COPY_REG(r8);
  COPY_REG(r9);
  COPY_REG(r10);
  COPY_REG(r11);
  COPY_REG(r12);
#undef COPY_REG
  dest->stack_ptr = mctx->arm_sp;
  dest->lr = mctx->arm_lr;
  dest->prog_ctr = mctx->arm_pc;
  dest->cpsr = mctx->arm_cpsr;
#elif defined(__mips__)
#define COPY_REG(A,I) dest->A = (uint32_t)mctx->sc_regs[I]
  COPY_REG(zero,0);
  COPY_REG(at,1);
  COPY_REG(v0,2);
  COPY_REG(v1,3);
  COPY_REG(a0,4);
  COPY_REG(a1,5);
  COPY_REG(a2,6);
  COPY_REG(a3,7);
  COPY_REG(t0,8);
  COPY_REG(t1,9);
  COPY_REG(t2,10);
  COPY_REG(t3,11);
  COPY_REG(t4,12);
  COPY_REG(t5,13);
  COPY_REG(t6,14);
  COPY_REG(t7,15);
  COPY_REG(s0,16);
  COPY_REG(s1,17);
  COPY_REG(s2,18);
  COPY_REG(s3,19);
  COPY_REG(s4,20);
  COPY_REG(s5,21);
  COPY_REG(s6,22);
  COPY_REG(s7,23);
  COPY_REG(t8,24);
  COPY_REG(t9,25);
  COPY_REG(k0,26);
  COPY_REG(k1,27);
  COPY_REG(global_ptr,28);
  COPY_REG(stack_ptr,29);
  COPY_REG(frame_ptr,30);
  COPY_REG(return_addr,31);
#undef COPY_REG
  dest->prog_ctr=(uint32_t)mctx->sc_pc;
#else
# error Unsupported architecture
#endif
}

static void nonsfi_exception_frame_from_signal_context(
    struct NonSfiExceptionFrame *frame,
    const void *raw_ctx) {
  const struct linux_ucontext_t *uctx = (struct linux_ucontext_t *) raw_ctx;
  const linux_mcontext_t *mctx = &uctx->uc_mcontext;
  frame->context.size = (((uintptr_t) (&frame->portable + 1))
                         - (uintptr_t) &frame->context);
  frame->context.portable_context_offset = ((uintptr_t) &frame->portable
                                            - (uintptr_t) &frame->context);
  frame->context.portable_context_size = sizeof(frame->portable);
  frame->context.regs_size = sizeof(frame->context.regs);

  memset(frame->context.reserved, 0, sizeof(frame->context.reserved));
  machine_context_to_register(mctx, &frame->context.regs);
  frame->portable.prog_ctr = frame->context.regs.prog_ctr;
  frame->portable.stack_ptr = frame->context.regs.stack_ptr;

#if defined(__i386__)
  frame->context.arch = EM_386;
  frame->portable.frame_ptr = frame->context.regs.ebp;
#elif defined(__arm__)
  frame->context.arch = EM_ARM;
  /* R11 is frame pointer in ARM mode, R8 is frame pointer in thumb mode. */
  frame->portable.frame_ptr = frame->context.regs.r11;
#elif defined(__mips__)
  frame->context.arch = EM_MIPS;
  frame->portable.frame_ptr = frame->context.regs.frame_ptr;
#else
# error Unsupported architecture
#endif
}

/* A replacement of sigreturn. It does not restore the signal mask. */
static void __attribute__((noreturn))
nonsfi_restore_context(const linux_mcontext_t *mctx) {

#if defined(__i386__)

#define OFFSET(name) \
  [name] "i" (offsetof(linux_mcontext_t, name))
#define RESTORE_SEGMENT(name) \
  "mov %c[" #name "](%%eax), %%" #name "\n"
#define RESTORE(name) \
  "movl %c[" #name "](%%eax), %%e" #name "\n"

  __asm__ __volatile__(
      /* TODO(lhchavez): Restore floating-point environment */

      /* Restore all segment registers */
      RESTORE_SEGMENT(gs)
      RESTORE_SEGMENT(fs)
      RESTORE_SEGMENT(es)
      RESTORE_SEGMENT(ds)

      /*
       * Restore most of the other registers.
       */
      RESTORE(di)
      RESTORE(si)
      RESTORE(bp)
      RESTORE(bx)

      /*
       * Prepare the last registers. eip *must* be one slot above the original
       * stack, since that is the only way eip and esp can be simultaneously
       * restored. Here, we are using ecx as the pseudo stack pointer, and edx
       * as a scratch register. Once the stack is laid out the way we want it to
       * be, restore edx and eax last.
       */
      "mov %c[sp](%%eax), %%ecx\n"
      "mov %c[ip](%%eax), %%edx\n"
      "mov %%edx, -4(%%ecx)\n"
      "mov %c[flags](%%eax), %%edx\n"
      "mov %%edx, -8(%%ecx)\n"
      "mov %c[cx](%%eax), %%edx\n"
      "mov %%edx, -12(%%ecx)\n"
      RESTORE(dx)
      RESTORE(ax)
      "lea -12(%%ecx), %%esp\n"

      /*
       * Finally pop ecx off the stack, restore the processor flags, and return
       * to simultaneously restore esp and eip.
       */
      "pop %%ecx\n"
      "popf\n"
      "ret\n"
      :
      : "a" (mctx),
        OFFSET(gs),
        OFFSET(fs),
        OFFSET(es),
        OFFSET(ds),
        OFFSET(di),
        OFFSET(si),
        OFFSET(bp),
        OFFSET(sp),
        OFFSET(bx),
        OFFSET(dx),
        OFFSET(cx),
        OFFSET(ax),
        OFFSET(ip),
        OFFSET(flags),
        OFFSET(fpstate)
  );

#undef OFFSET
#undef RESTORE
#undef RESTORE_SEGMENT

#elif defined(__arm__)

#define OFFSET(name) \
  [name] "I" (offsetof(linux_mcontext_t, arm_ ## name) - \
              offsetof(linux_mcontext_t, arm_r0))

  register uint32_t a14 __asm__("r14") = (uint32_t) &mctx->arm_r0;

  __asm__ __volatile__(
      /* Restore flags */
      "ldr r0, [r14, %[cpsr]]\n"
      "msr APSR_nzcvqg, r0\n"

      /* TODO(lhchavez): Restore floating-point environment */

      /*
       * Restore general-purpose registers.
       * This code does not use the simpler 'ldmia r14, {r0-pc}' since using
       * ldmia with either sp or with both lr and pc is deprecated.
       */
      "ldmia r14, {r0-r10}\n"

      /*
       * Copy r11, r12, lr, and pc just before the original sp.
       * r12 will work as a temporary sp. r11 will be the scratch register, and
       * will be restored just before moving sp.
       */
      "ldr r12, [r14, %[sp]]\n"

      "ldr r11, [r14, %[pc]]\n"
      "stmdb r12!, {r11}\n"
      "ldr r11, [r14, %[lr]]\n"
      "stmdb r12!, {r11}\n"
      "ldr r11, [r14, %[r12]]\n"
      "stmdb r12!, {r11}\n"
      "ldr r11, [r14, %[r11]]\n"
      "mov sp, r12\n"

      /*
       * Restore r12, lr, and pc. sp will point to the correct location once
       * we're done.
       */
      "pop {r12, lr}\n"
      "pop {pc}\n"
      :
      : "r" (a14),
        OFFSET(cpsr),
        OFFSET(r11),
        OFFSET(r12),
        OFFSET(sp),
        OFFSET(lr),
        OFFSET(pc)
  );

#undef OFFSET
#elif defined(__mips__)

#define SCOFFSET(name) \
  [name]"i"(offsetof(linux_mcontext_t, sc_##name))

#define REGSIZE(regs) sizeof(ctx->sc_##regs[0])

#define GPOFFSET(n) \
  [gp##n]"i"(offsetof(linux_mcontext_t, sc_regs) + (n * REGSIZE(regs)))

#define GPRESTORE(n) \
  "lw $"#n", %[regs]+"#n"*%[reg_size](%0)\n"

#define FPRESTORE(n) \
  "lwc1 $f"#n", %[fpregs]+"#n"*%[fpreg_size](%0)\n"

#define GP2STASH (-28)

#define PCSTASH  (-32)

#define READ_FROM_STASH(name,off) \
   ctx->name = *((uint32_t*)((char*)(ctx->sc_regs[29]) + off))

  register linux_mcontext_t* ctx __asm__("$4") = (linux_mcontext_t *) mctx;

  /*
   * Checks if we interrupted another signal handler at point when all
   * registers except $2 and $pc have been restored and  $2's and $pc's
   * original values are stashed above user stack.
   * If true we copy $2's and $pc's original values to context so we can
   * finish the restoration.
   */
  if((uint32_t)mctx->sc_pc >= (uint32_t)g_restore_gp2_pc_start &&
     (uint32_t)mctx->sc_pc <= (uint32_t)g_restore_gp2_pc_end) {
    READ_FROM_STASH(sc_regs[2],GP2STASH);
    READ_FROM_STASH(sc_pc,PCSTASH);
  }
  __asm__ __volatile__(
    ".set push\n"
    ".set noreorder\n"
    ".set noat\n"
    /* Restore floating point registers */
    FPRESTORE(0)
    FPRESTORE(1)
    FPRESTORE(2)
    FPRESTORE(3)
    FPRESTORE(4)
    FPRESTORE(5)
    FPRESTORE(6)
    FPRESTORE(7)
    FPRESTORE(8)
    FPRESTORE(9)
    FPRESTORE(10)
    FPRESTORE(11)
    FPRESTORE(12)
    FPRESTORE(13)
    FPRESTORE(14)
    FPRESTORE(15)
    FPRESTORE(16)
    FPRESTORE(17)
    FPRESTORE(18)
    FPRESTORE(19)
    FPRESTORE(20)
    FPRESTORE(21)
    FPRESTORE(22)
    FPRESTORE(23)
    FPRESTORE(24)
    FPRESTORE(25)
    FPRESTORE(26)
    FPRESTORE(27)
    FPRESTORE(28)
    FPRESTORE(29)
    FPRESTORE(30)
    FPRESTORE(31)
    /*
     * Restore all user mode general-purpose registers except
     * $2, $3, $4 , $29 . $0 doesn't need restoring
     */
    GPRESTORE(1)
    GPRESTORE(5)
    GPRESTORE(6)
    GPRESTORE(7)
    GPRESTORE(8)
    GPRESTORE(9)
    GPRESTORE(10)
    GPRESTORE(11)
    GPRESTORE(12)
    GPRESTORE(13)
    GPRESTORE(14)
    GPRESTORE(15)
    GPRESTORE(16)
    GPRESTORE(17)
    GPRESTORE(18)
    GPRESTORE(19)
    GPRESTORE(20)
    GPRESTORE(21)
    GPRESTORE(22)
    GPRESTORE(23)
    GPRESTORE(24)
    GPRESTORE(25)
    GPRESTORE(28)
    GPRESTORE(30)
    GPRESTORE(31)
    /*
     * Restore mdhi and mdlo.
     * We stash gp2 and pc "registers" values on original stack while using $2
     * as temporary sp and $3 as scratch registers.
     * Next we restore $3, $4 and in the end $29($sp)
     * After end of this block original stack has been restored and stashed
     * values of gp2  and pc are located at positions $sp-32 and $sp-28
     * above sp respectively.
     * Signal handling frame starts above sp at $sp-32 so thease values won't be
     * clobbered if we get interrupted after this point.
     * Note that memory at top of stack from $sp to $sp-24 might be in use by
     * FPU emulator, so we avoid using that memory range.
     */
    "lw $2, %[mdhi](%0)\n"
    "lw $3, %[mdlo](%0)\n"
    "mfhi $2\n"
    "lw $2, %[gp29](%0)\n"
    "mflo $3\n"
    "lw $3, %[gp2](%0)\n"
    "sw $3, %[gp2stash]($2)\n"
    "lw $3, %[pc](%0)\n"
    "sw $3, %[pcstash]($2)\n"
    "lw $3, %[gp3](%0)\n"
    "lw %0, %[gp4](%0)\n"
    "move $29, $2\n"
    ".global g_restore_gp2_pc_start\n"
    "g_restore_gp2_pc_start:\n"
    /*
     * We load original $pc's value from stashed location to $2 and perform
     * indirect jump while restoring original $2's value in delay slot.
     * If we get interrupted by another signal in the middle of this the
     * check  at restorer's start ensures that nested signal restorer will
     * finish our job.
     */
    "lw $2, %[pcstash]($29)\n"
    "jr $2\n"
    "lw $2, %[gp2stash]($29)\n"
    ".global g_restore_gp2_pc_end\n"
    "g_restore_gp2_pc_end:\n"
    ".set pop\n"
    :
    :"r"(ctx),
     SCOFFSET(regs),
     SCOFFSET(fpregs),
     SCOFFSET(pc),
     SCOFFSET(mdhi),
     SCOFFSET(mdlo),
     GPOFFSET(2),
     GPOFFSET(3),
     GPOFFSET(4),
     GPOFFSET(29),
     [reg_size]"i"(REGSIZE(regs)),
     [fpreg_size]"i"(REGSIZE(fpregs)),
     [gp2stash]"i"(GP2STASH),
     [pcstash]"i"(PCSTASH)
  );

#undef SCOFFSET
#undef GPOFFSET
#undef REGSIZE
#undef FPRESTORE
#undef GPRESTORE
#undef READ_FROM_STASH

#else
# error Unsupported architecture
#endif

  /* Should never reach this. */
  __builtin_trap();
}

static __attribute__((noreturn))
void restore_context(void *raw_ctx) {
  const struct linux_ucontext_t *uctx = (struct linux_ucontext_t *) raw_ctx;
  const linux_mcontext_t *mctx = &uctx->uc_mcontext;
  nonsfi_restore_context(mctx);
}

/* Signal handlers, responsible for calling the registered handlers. */
static void exception_catch(int sig, linux_siginfo_t *info, void *uc) {
  if (g_exception_handler_function_pointer) {
    struct NonSfiExceptionFrame exception_frame;
    nonsfi_exception_frame_from_signal_context(&exception_frame, uc);
    g_exception_handler_function_pointer(&exception_frame.context);
  }
  _exit(-sig);
}

static void signal_catch(int sig, linux_siginfo_t *info, void *uc) {
  if (g_signal_handler_function_pointer) {
    struct NonSfiExceptionFrame exception_frame;
    nonsfi_exception_frame_from_signal_context(&exception_frame, uc);
    g_signal_handler_function_pointer(&exception_frame.context);
  }
  restore_context(uc);
}

static void nonsfi_install_exception_handler_locked() {
  struct linux_sigaction sa;
  unsigned int a;

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = exception_catch;
  sa.sa_flags = LINUX_SA_SIGINFO | LINUX_SA_ONSTACK;

  /*
   * Reuse the sigemptyset/sigaddset for the first 32 bits of the
   * sigmask. Works on little endian systems only.
   */
  sigset_t *mask = (sigset_t*)&sa.sa_mask;
  sigemptyset(mask);

  /* Mask all signals we catch to prevent re-entry. */
  for (a = 0; a < NACL_ARRAY_SIZE(kSignals); a++) {
    sigaddset(mask, kSignals[a]);
  }

  /* Install all handlers. */
  for (a = 0; a < NACL_ARRAY_SIZE(kSignals); a++) {
    if (linux_sigaction(kSignals[a], &sa, NULL) != 0)
      abort();
  }
}

static void nonsfi_install_signal_handler_locked() {
  struct linux_sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = signal_catch;

  /*
   * User signal handler can be recursively interrupted to avoid having
   * to allow sigreturn/sigprocmask.
   */
  sa.sa_flags = LINUX_SA_SIGINFO | LINUX_SA_NODEFER | LINUX_SA_RESTART;
  sigset_t *mask = (sigset_t*)&sa.sa_mask;
  sigemptyset(mask);

  /*
   * Install a single handler.  Multiple signals can be multiplexed in
   * userspace.
   */
  if (linux_sigaction(LINUX_SIGUSR1, &sa, NULL) != 0)
    abort();
}

static void nonsfi_initialize_signal_handler_locked() {
  if (g_signal_handler_initialized)
    return;
  pid_t tgid = getpid();
  if (tgid == -1)
    abort();
  pid_t main_tid = syscall(__NR_gettid);
  if (main_tid == -1)
    abort();
  nonsfi_install_exception_handler_locked();
  nonsfi_install_signal_handler_locked();
  g_tgid = tgid;
  g_main_tid = main_tid;
  g_signal_handler_initialized = 1;
}

/*
 * Initialize signal handlers before entering sandbox.
 */
void nonsfi_initialize_signal_handler() {
  if (pthread_mutex_lock(&g_mutex) != 0)
    abort();
  nonsfi_initialize_signal_handler_locked();
  if (pthread_mutex_unlock(&g_mutex) != 0)
    abort();
}

int nacl_exception_get_and_set_handler(NaClExceptionHandler handler,
                                       NaClExceptionHandler *old_handler) {
  if (pthread_mutex_lock(&g_mutex) != 0)
    abort();
  nonsfi_initialize_signal_handler_locked();
  if (old_handler)
    *old_handler = g_exception_handler_function_pointer;
  g_exception_handler_function_pointer = handler;
  if (pthread_mutex_unlock(&g_mutex) != 0)
    abort();
  return 0;
}

int nacl_exception_set_handler(NaClExceptionHandler handler) {
  return nacl_exception_get_and_set_handler(handler, NULL);
}

int nacl_exception_clear_flag(void) {
  /*
   * Unblock signals, useful for unit testing and continuing to
   * process after fatal signal.
   */

  /* Allocate the 8 bytes of signal mask. */
  linux_sigset_t mask;

  /*
   * sigemptyset will only clear first 4 bytes of sigset_t, and
   * compat_sigset_t has 8 bytes, clear with memset.
   */
  memset(&mask, 0, sizeof(mask));

  /*
   * Hack to be able to reuse sigset_t utilities from newlib for the
   * first lower 4 bytes of the signal, works because we are all
   * little endians.
   */
  sigset_t *maskptr = (sigset_t *) &mask;
  sigemptyset(maskptr);
  for (size_t a = 0; a < NACL_ARRAY_SIZE(kSignals); a++) {
    if (sigaddset(maskptr, kSignals[a]) != 0)
      abort();
  }
  if (linux_sigprocmask(LINUX_SIG_UNBLOCK, &mask, NULL) != 0)
    abort();

  return 0;
}

int nacl_exception_set_stack(void *p, size_t s) {
  /* Not implemented yet. */
  return ENOSYS;
}

int nacl_async_signal_set_handler(NaClIrtAsyncSignalHandler handler) {
  if (pthread_mutex_lock(&g_mutex) != 0)
    abort();
  nonsfi_initialize_signal_handler_locked();
  g_signal_handler_function_pointer = handler;
  if (pthread_mutex_unlock(&g_mutex) != 0)
    abort();
  return 0;
}

int nacl_async_signal_send_async_signal(nacl_irt_tid_t tid) {
  if (!g_signal_handler_initialized)
    return ESRCH;
  if (tid == 0)
    tid = g_main_tid;
  if (linux_tgkill(g_tgid, tid, LINUX_SIGUSR1) == -1)
    return errno;
  return 0;
}
