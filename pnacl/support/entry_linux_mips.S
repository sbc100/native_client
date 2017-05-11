/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

    /*
     * This routine implements a Linux/ELF entry point, where argc/argv and
     * the environment are passed on the stack.  This startup protocol is
     * not compatible with normal C function calling conventions.  This
     * routine converts to C calling conventions by passing the stack
     * pointer to __pnacl_start_linux_c().
     *
     * The ELF ABI says that v0, if non-zero, points to a finalization
     * function (for registering with atexit()).  That is only set by the
     * dynamic linker, not by the kernel, so we ignore it here.
     */
    .globl __pnacl_start_linux
    .type __pnacl_start_linux, @function
    .ent  __pnacl_start_linux
    .set push
    .option pic2
    .set noreorder
__pnacl_start_linux:
    /*
     * Zero the return address register to make the outermost frame
     * more obvious to debuggers.
     */
    move $ra, $zero
    move $a0, $sp /* Set up argument */
    b __pnacl_start_linux_c
    addiu $sp, $sp, -16


    .set pop
    .end __pnacl_start_linux

