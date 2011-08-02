/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator_x86/nc_decode_tables.h"

#include "native_client/src/trusted/validator_x86/ncopcode_desc.h"
#if NACL_TARGET_SUBARCH == 64
# include "native_client/src/trusted/validator_x86/gen/nc_opcode_table_64.h"
#else
# include "native_client/src/trusted/validator_x86/gen/nc_opcode_table_32.h"
#endif

static const NaClDecodeTables kDecoderTables = {
  g_Operands,
  g_Opcodes + 0,
  g_Opcodes + 0,
  (const NaclInstTableType*)(&g_OpcodeTable),
  kNaClPrefixTable,
  g_OpcodeSeq
};

const struct NaClDecodeTables* kNaClDecoderTables = &kDecoderTables;
