/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This is the native crtbegin (crtbegin.o).
 *
 * It contains the constructor/destructor for exception handling,
 * and the symbol for __EH_FRAME_BEGIN__. This is native because
 * exception handling is also native (externally provided).
 *
 * We also need to sneak in the native PPAPI shim library between
 * these bookends because otherwise the unwind tables will be messed
 * up, c.f.
 * https://chromiumcodereview.appspot.com/9909016/
 * TODO(robertm): see whether this problem goes away once we switch
 * eh_frame_headers
 */

/* This goes first to ensure that tls_params.h hasn't been #included before. */
#define NACL_DEFINE_EXTERNAL_NATIVE_SUPPORT_FUNCS
#include "native_client/src/untrusted/nacl/pnacl.h"
#include "native_client/src/untrusted/nacl/tls_params.h"

#include <stdint.h>

#include "native_client/pnacl/support/pnacl_irt.h"
#include "native_client/src/include/elf32.h"


void _pnacl_wrapper_start(uint32_t *info);

/*
 * HACK:
 * The real structure is defined in unwind-dw2-fde.h
 * this is something that is at least twice as big.
 */
struct object {
  void *p[16] __attribute__((aligned(8)));
};

/*
 * __[de]register_frame_info() are provided by libgcc_eh. When not linking
 * with libgcc_eh, dummy implementations are provided.
 * See: gcc/unwind-dw2-fde.c
 */

#ifdef LINKING_WITH_LIBGCC_EH
extern void __register_frame_info(void *begin, struct object *ob);
extern void __deregister_frame_info(const void *begin);
#else /* not LINKING_WITH_LIBGCC_EH */
void __register_frame_info(void *begin, struct object *ob) {}
void __deregister_frame_info(const void *begin) {}
#endif /* LINKING_WITH_LIBGCC_EH */


/*
 * Exception handling frames are aggregated into a single section called
 * .eh_frame.  The runtime system needs to (1) have a symbol for the beginning
 * of this section, and needs to (2) mark the end of the section by a NULL.
 */

static char __EH_FRAME_BEGIN__[]
    __attribute__((section(".eh_frame"), aligned(4)))
    = { };

static void __do_eh_ctor(void) {
  static struct object object;
  __register_frame_info (__EH_FRAME_BEGIN__, &object);
}

#if defined(__native_client_nonsfi__)
#if defined(__mips__)
/*
 * MIPS: All function call got through GOT which hasn't been relocated
 * at this point. So we have to use inline assembley to determite nexe
 * load address and calculate correct addresses of apply_relocations function
 * and .dynamic section.
 */
static inline void  apply_relocations() {
  void (*apply_relocations)(const Elf32_Dyn*, uintptr_t);
  uintptr_t load_bias;
  const Elf32_Dyn* dyn;
  __asm__ (".set push\n"
           ".option pic2\n"
           ".set noreorder\n"
           "bal 0f\n" /* $ra(31) = $pc + 8 */
           "nop\n"
           "0:\n"
           ".cpload $31\n" /* Setup $gp(28) regitser */
           "lw %1, %%got(0b)($28)\n"
           "addiu %1, %1, %%lo(0b)\n"
           /* Load address value of .0 label from GOT.
            * Prior to relocation value contained in GOT represents offset
            * of .0 label from start of nexe.
            * We use that and runtime address of .0 label ($31) to calculate
            * nexe's load address.
            */
           "subu %1, $31, %1\n"
           "lw %0, %%got(apply_relocations)($28)\n"
           "addu %0, %0, %1\n"
           "lw %2, %%got(_DYNAMIC)($28)\n"
           "addu %2, %2, %1\n"
           ".set pop\n"
           : "=r" (apply_relocations),
             "=r" (load_bias),
             "=r" (dyn)
           :
           :"$28","$31");
  apply_relocations(dyn,load_bias);
}
#else
/*
 * x86 and ARM: Using "hidden" visibility ensures these symbols aren't
 * referenced through the GOT, which hasn't been relocated yet.
 */
extern const Elf32_Dyn _DYNAMIC[] __attribute__((visibility("hidden")));
extern char _begin[] __attribute__((visibility("hidden")));

extern void apply_relocations(const Elf32_Dyn*, uintptr_t);

#endif
#endif


/* This defines the entry point for a nexe produced by the PNaCl translator. */
void __pnacl_start(uint32_t *info) {
#if defined(__native_client_nonsfi__)
#if defined(__mips__)
  apply_relocations();
#else
  apply_relocations(_DYNAMIC,(uintptr_t)_begin);
#endif
#endif

  __pnacl_init_irt(info);

  /*
   * We must register exception handling unwind info before calling
   * any user code.  Note that we do not attempt to deregister the
   * unwind info at exit, but there is no particular need to do so.
   */
  __do_eh_ctor();

  _pnacl_wrapper_start(info);
}
