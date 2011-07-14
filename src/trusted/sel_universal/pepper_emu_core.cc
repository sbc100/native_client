// Copyright (c) 2011 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <stdio.h>
#include <string>

// Note: this file defines hooks for all pepper related srpc calls
// it would be nice to keep this synchronized with
// src/shared/ppapi_proxy/ppb_rpc_server.cc
// which is a generated file
#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/sel_universal/pepper_emu.h"
#include "native_client/src/trusted/sel_universal/primitives.h"
#include "native_client/src/trusted/sel_universal/rpc_universal.h"
#include "native_client/src/trusted/sel_universal/srpc_helper.h"

using std::string;

namespace {

// we currently only use this for pushing events upstream
IMultimedia* GlobalMultiMediaInterface = 0;
// ======================================================================
// NOTE: These are not fully supported at this time.
//       They undoubtedly need to be updated when ppapi changes.
//       We do not use defines like PPB_CORE_INTERFACE because
//       the implementation/emulation needs to be updated as well.
bool IsSupportedInterface(string if_name) {
  return
    if_name == "PPB_Audio;0.6" ||
    if_name == "PPB_AudioConfig;0.5" ||
    if_name == "PPB_Core;0.5" ||
    if_name == "PPB_FileIO(Dev);0.4" ||
    if_name == "PPB_Graphics2D;0.4" ||
    if_name == "PPB_ImageData;0.3" ||
    if_name == "PPB_Instance;0.5" ||
    if_name == "PPB_Messaging;0.1" ||
    if_name == "PPB_URLLoader;0.2" ||
    if_name == "PPB_URLRequestInfo;0.2" ||
    if_name == "PPB_URLResponseInfo;0.1" ||
    if_name == "PPB_Var(Deprecated);0.3" ||
    if_name == "PPB_Var;0.5";
}

// void* PPB_GetInterface(const char* interface_name);
// PPB_GetInterface:s:i
void PPB_GetInterface(SRPC_PARAMS) {
  string if_name(ins[0]->arrays.str);
  NaClLog(1, "PPB_GetInterface(%s)\n", if_name.c_str());
  bool supported = IsSupportedInterface(if_name);
  if (!supported) {
    NaClLog(LOG_ERROR, "unsupported interface [%s]\n", if_name.c_str());
  }
  outs[0]->u.ival = supported ? 1 : 0;
  NaClLog(1, "PPB_GetInterface(%s) -> %d\n", if_name.c_str(), supported);
  rpc->result = NACL_SRPC_RESULT_OK;
  done->Run(done);
}

// From the Core API
// void ReleaseResource(PP_Resource resource);
// PPB_Core_ReleaseResource:i:
void PPB_Core_ReleaseResource(SRPC_PARAMS) {
  UNREFERENCED_PARAMETER(ins);
  UNREFERENCED_PARAMETER(outs);
  NaClLog(1, "PPB_Core_ReleaseResource\n");
  rpc->result = NACL_SRPC_RESULT_OK;
  done->Run(done);
}

// From the Core API
// void CallOnMainThread(int32_t delay_in_milliseconds,
//                       struct PP_CompletionCallback callback,
//                       int32_t result);
// PPB_Core_CallOnMainThread:iii:
void PPB_Core_CallOnMainThread(SRPC_PARAMS) {
  UNREFERENCED_PARAMETER(outs);
  const int delay = ins[0]->u.ival;
  const int callback = ins[1]->u.ival;
  const int result = ins[2]->u.ival;

  NaClLog(1, "PPB_Core_CallOnMainThread(%d, %d, %d)\n",
          delay, callback, result);

  rpc->result = NACL_SRPC_RESULT_OK;
  done->Run(done);

  PP_InputEvent event;
  MakeUserEvent(&event, CUSTOM_EVENT_TIMER_CALLBACK, callback, result, 0, 0);
  GlobalMultiMediaInterface->PushDelayedUserEvent(delay, &event);
}

// This appears to have no equivalent in the ppapi world
// ReleaseResourceMultipleTimes:ii:
void ReleaseResourceMultipleTimes(SRPC_PARAMS) {
  UNREFERENCED_PARAMETER(outs);
  NaClLog(1, "ReleaseResourceMultipleTimes(%d, %d)\n",
          ins[0]->u.ival, ins[1]->u.ival);
  rpc->result = NACL_SRPC_RESULT_OK;
  done->Run(done);
}

}  // end namespace


#define TUPLE(a, b) #a #b, a
void PepperEmuInitCore(NaClCommandLoop* ncl, IMultimedia* im) {
  GlobalMultiMediaInterface = im;
  // Register Core and misc interfaces
  ncl->AddUpcallRpc(TUPLE(PPB_Core_ReleaseResource, :i:));
  ncl->AddUpcallRpc(TUPLE(PPB_GetInterface, :s:i));
  ncl->AddUpcallRpc(TUPLE(ReleaseResourceMultipleTimes, :ii:));
  ncl->AddUpcallRpc(TUPLE(PPB_Core_CallOnMainThread, :iii:));
  // This is the only rpc for now that can be called from
  // a nexe thread other than main
  // c.f. src/shared/ppapi_proxy/upcall_server.cc
  ncl->AddUpcallRpcSecondary(TUPLE(PPB_Core_CallOnMainThread, :iii:));
}
