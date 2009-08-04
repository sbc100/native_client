{
  'targets': [
    {
      'target_name': 'pull_in_all',
      'type': 'none',
      # NOTE: Chrome-specific targets should not be part of this project
      'dependencies': [
        '../src/shared/npruntime/npruntime.gyp:*',
        '../src/shared/imc/imc.gyp:*',
        '../src/shared/srpc/srpc.gyp:*',
        '../src/shared/utils/utils.gyp:*',
        '../src/trusted/desc/desc.gyp:*',
        '../src/trusted/nonnacl_util/nonnacl_util.gyp:*',
        '../src/trusted/platform/platform.gyp:*',
        '../src/trusted/platform_qualify/platform_qualify.gyp:*',
        '../src/trusted/sandbox/sandbox.gyp:*',
        '../src/trusted/plugin/plugin.gyp:npGoogleNaClPlugin',
        '../src/trusted/service_runtime/service_runtime.gyp:*',
        '../src/trusted/validator_x86/validator_x86.gyp:*',
      ],
    },
  ],
}
