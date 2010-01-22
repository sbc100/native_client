deps = {
  "tools/gyp":
    "http://gyp.googlecode.com/svn/trunk@636",
  "build":
    "http://src.chromium.org/svn/trunk/src/build@36563",
  "site_scons":
    "http://src.chromium.org/svn/trunk/src/site_scons@36563",
  "testing/gtest":
    "http://googletest.googlecode.com/svn/trunk@267",
  "third_party":
    "http://nativeclient.googlecode.com/svn/trunk/src/third_party@782",
}

hooks = [
  {
    "pattern": "\\.gypi?$|[/\\\]tools[/\\\]gyp[/\\\]",
    "action": ["python", "native_client/build/gyp_nacl",
               "native_client/build/all.gyp"],
  },
]
