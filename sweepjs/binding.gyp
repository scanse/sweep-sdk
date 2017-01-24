{
    "includes": [ "common.gypi" ],
    "targets": [
        {
            "target_name": "sweepjs",
            "sources": [ "sweepjs.cc" ],
            "include_dirs": ["<!(node -e \"require('nan')\")"],
            "link_settings": { "libraries": ["-lsweep"], },
            "xcode_settings": {
                "GCC_ENABLE_CPP_RTTI": "YES",
                "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                "MACOSX_DEPLOYMENT_TARGET": "10.8",
                "CLANG_CXX_LIBRARY": "libc++",
                "CLANG_CXX_LANGUAGE_STANDARD": "c++11",
                "GCC_VERSION": "com.apple.compilers.llvm.clang.1_0"
            },
        },
    ]
}
