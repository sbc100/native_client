/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// C/C++ library for handler passing in the Windows Chrome sandbox.
// sel_ldr side interface.

#include <map>
#include "native_client/src/trusted/handle_pass/ldr_handle.h"

#include "native_client/src/trusted/handle_pass/handle_lookup.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"

static const HANDLE kInvalidHandle = reinterpret_cast<HANDLE>(-1);

static struct NaClDesc* lookup_desc = NULL;
static NaClSrpcChannel lookup_channel;

// All APIs are guarded by the single mutex.
static struct NaClMutex pid_handle_map_mu = { NULL };

// The map.
static std::map<DWORD, HANDLE>* local_pid_handle_map = NULL;

void NaClHandlePassLdrInit() {
  NaClMutexCtor(&pid_handle_map_mu);
}

int NaClHandlePassLdrCtor(struct NaClDesc* socket_address,
                          DWORD renderer_pid,
                          NaClHandle renderer_handle) {
  NaClMutexLock(&pid_handle_map_mu);
  local_pid_handle_map = new(std::nothrow) std::map<DWORD, HANDLE>;
  if (NULL == local_pid_handle_map) {
    NaClMutexUnlock(&pid_handle_map_mu);
    return 0;
  }
  (*local_pid_handle_map)[renderer_pid] = renderer_handle;
  NaClMutexUnlock(&pid_handle_map_mu);
  NaClHandlePassSetLookupMode(HANDLE_PASS_CLIENT_PROCESS);

  // Connect to the given socket address.
  if (0 == (socket_address->vtbl->ConnectAddr)(socket_address, &lookup_desc)) {
    return 0;
  }
  // Create an SRPC client for lookup requests.
  if (!NaClSrpcClientCtor(&lookup_channel, lookup_desc)) {
    NaClDescUnref(lookup_desc);
    return 0;
  }
  // Success.
  return 1;
}

HANDLE NaClHandlePassLdrLookupHandle(DWORD pid) {
  int int_handle;
  HANDLE retval = NULL;  // return NULL on error - that's what IMC checks for.
  NaClMutexLock(&pid_handle_map_mu);
  if (NULL == local_pid_handle_map)
    goto mutex_locked;

  if (local_pid_handle_map->end() != local_pid_handle_map->find(pid)) {
    // We have the handle in the cache
    retval = (*local_pid_handle_map)[pid];
    goto mutex_locked;
  }
  NaClMutexUnlock(&pid_handle_map_mu);

  // Optimization: try lookup in a local map first.
  if (NACL_SRPC_RESULT_OK != NaClSrpcInvokeBySignature(&lookup_channel,
                                                       "lookup:ii:i",
                                                       GetCurrentProcessId(),
                                                       pid,
                                                       &int_handle)) {
    return NULL;
  }
  NaClMutexLock(&pid_handle_map_mu);
  retval = reinterpret_cast<HANDLE>(int_handle);
  (*local_pid_handle_map)[pid] = retval;
 mutex_locked:
  NaClMutexUnlock(&pid_handle_map_mu);
  return retval;
}

void NaClHandlePassLdrDtor() {
  // TODO(gregoryd, sehr): this function should check whether the channel
  // has been initialized.
  // Tell the server to shut down the thread used for this sel_ldr process.
  NaClSrpcInvokeBySignature(&lookup_channel, "shutdown::");
  // And destroy the SRPC client (which also unrefs lookup_desc).
  NaClSrpcDtor(&lookup_channel);
}
