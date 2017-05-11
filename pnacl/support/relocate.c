/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf_constants.h"


#if defined(__native_client_nonsfi__)

static int find_dt_entry(const Elf32_Dyn* dyn, int dt_tag,
                                uintptr_t *result) {
  for (; dyn->d_tag != DT_NULL; dyn++) {
    if (dyn->d_tag == dt_tag) {
      *result = dyn->d_un.d_val;
      return 1;
    }
  }
  return 0;
}

/*
 * It is not straightforward to print a failure message at this point
 * (it would require querying the IRT for write()), so crash instead.
 */
#if !defined(__mips__)
static __attribute__((noinline))
#else
/*
 * On MIPS we cannot call non-inline functions until we finish relocating.
 * This file is built with -O3 so static modifier is enough.
 */
static
#endif
void unhandled_relocation(void) {
  __builtin_trap();
}

#if defined(__mips__)
static  void mips_setup_got(const Elf32_Dyn* dyn, uintptr_t load_bias) {
 uintptr_t got_offset;
 uintptr_t local_gotno;

 if(!find_dt_entry(dyn, DT_PLTGOT, &got_offset) ||
    !find_dt_entry(dyn, DT_MIPS_LOCAL_GOTNO, &local_gotno))
    __builtin_trap();

  uint32_t* got = (uint32_t*) (load_bias + got_offset);
  /* First two entries are resevred for use by ld.so */
  uint32_t* local_got = got + 2;
  uint32_t* local_got_end = got + local_gotno;

  /* Add load address displacement to all local GOT entries */
  for(; local_got < local_got_end; local_got++)
     *local_got += load_bias;

  /*
   * Nothing to do for global entries.
   * We asume that all global entries are for weak symbols left by linker
   */
}
#endif

void apply_relocations(const Elf32_Dyn* dyn, uintptr_t load_bias) {
#if defined(__mips__)
  mips_setup_got(dyn,load_bias);
#endif

  uintptr_t rel_offset;
  uintptr_t rel_size;
  if (!find_dt_entry(dyn, DT_REL, &rel_offset) ||
      !find_dt_entry(dyn, DT_RELSZ, &rel_size)) {
    /* Nothing to do.  A very simple nexe might have no relocations. */
    return;
  }
  const Elf32_Rel *rels = (const Elf32_Rel *) (load_bias + rel_offset);
  const Elf32_Rel *rels_end = (const Elf32_Rel *) (load_bias + rel_offset
                                                   + rel_size);
  for (; rels < rels_end; rels++) {
    uint32_t reloc_type = ELF32_R_TYPE(rels->r_info);
    uintptr_t addr = load_bias + rels->r_offset;
    switch (reloc_type) {
#if defined(__i386__)
      case R_386_RELATIVE:
#elif defined(__arm__)
      case R_ARM_RELATIVE:
#elif defined(__mips__)
      case R_MIPS_REL32:
#else
# error Unhandled architecture
#endif
        *(uint32_t *) addr += load_bias;
        break;
#if defined(__i386__)
      case R_386_GLOB_DAT:
#elif defined(__arm__)
      case R_ARM_GLOB_DAT:
#elif defined(__mips__)
      case R_MIPS_NONE:
#else
# error Unhandled architecture
#endif
#if !defined(__mips__)
        /*
         * Assume this was a weak symbol reference, because the linker
         * leaves these in.
         */
        *(uint32_t *) addr = 0;
#endif
        break;
      default:
        unhandled_relocation();
    }
  }
}
#endif
