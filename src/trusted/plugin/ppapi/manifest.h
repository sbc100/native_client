/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Manifest file processing class.

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLUGIN_PPAPI_MANIFEST_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLUGIN_PPAPI_MANIFEST_H_

#include <map>
#include <set>
#include <string>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_string.h"
#include "native_client/src/third_party_mod/jsoncpp/include/json/value.h"

namespace pp {
class URLUtil_Dev;
}  // namespace pp

namespace plugin {

class ErrorInfo;

class Manifest {
 public:
  Manifest(const pp::URLUtil_Dev* url_util,
           const nacl::string& manifest_base_url,
           const nacl::string& sandbox_isa)
      : url_util_(url_util),
      manifest_base_url_(manifest_base_url),
      sandbox_isa_(sandbox_isa),
      dictionary_(Json::nullValue) { }
  ~Manifest() { }

  // Initialize the manifest object for use by later lookups.  The return
  // value is true if the manifest parses correctly and matches the schema.
  bool Init(const nacl::string& json, ErrorInfo* error_info);

  // Gets the full program URL for the current sandbox ISA from the
  // manifest file. Sets |is_portable| to |true| if the program is
  // portable bitcode.
  bool GetProgramURL(nacl::string* full_url,
                     ErrorInfo* error_info,
                     bool* is_portable);

  // TODO(jvoung): Get rid of these when we find a better way to
  // store / install these.
  // Gets the full nexe URL for the LLC nexe from the manifest file.
  bool GetLLCURL(nacl::string* full_url, ErrorInfo* error_info);

  // Gets the full nexe URL for the LD nexe from the manifest file.
  bool GetLDURL(nacl::string* full_url, ErrorInfo* error_info);
  // end TODO(jvoung)

  // Resolves a URL relative to the manifest base URL
  bool ResolveURL(const nacl::string& relative_url,
                  nacl::string* full_url,
                  ErrorInfo* error_info);

 private:
  const pp::URLUtil_Dev* url_util_;
  nacl::string manifest_base_url_;
  nacl::string sandbox_isa_;
  Json::Value dictionary_;

  // Checks that |dictionary_| is a valid manifest, according to the schema.
  // Returns true on success, and sets |error_info| to a detailed message
  // if not.
  bool MatchesSchema(ErrorInfo* error_info);

  NACL_DISALLOW_COPY_AND_ASSIGN(Manifest);
};


}  // namespace plugin

#endif  // NATIVE_CLIENT_SRC_TRUSTED_PLUGIN_PPAPI_MANIFEST_H_
