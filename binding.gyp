{
    "targets": [
        {
            "target_name": "RerunOBSBinding",
            "sources": [
                "src/RerunOBSBinding.cpp", "src/RerunOBSClient.cpp", "src/RerunOBSScene.cpp", "src/RerunOBSSource.cpp", 
                "src/RerunOBSSceneItem.cpp", "src/JSEventProvider.cpp", "src/JSEventProviderIntKeyed.cpp"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "obs/src/libobs"
            ],
            "libraries": [
                "../obs/bin/obs.lib"
            ],

            # Enable C++ -> JS exception propagation
            'cflags!': ['-fno-exceptions'],
            'cflags_cc!': ['-fno-exceptions'],
            'xcode_settings': {
                'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                'CLANG_CXX_LIBRARY': 'libc++',
                'MACOSX_DEPLOYMENT_TARGET': '10.7',
            },
            'msvs_settings': {
                'VCCLCompilerTool': {'ExceptionHandling': 1},
            },
            "defines": ["NAPI_CPP_EXCEPTIONS"]
        }
    ]
}
