{
    "includes": [ "common.gypi" ],
    "targets": [
        {
            "target_name": "sweepjs",
            "sources": [ "sweepjs.cc" ],
            "xcode_settings": {
                "GCC_ENABLE_CPP_RTTI": "YES",
                "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                "MACOSX_DEPLOYMENT_TARGET": "10.8",
                "CLANG_CXX_LIBRARY": "libc++",
                "CLANG_CXX_LANGUAGE_STANDARD": "c++11",
                "GCC_VERSION": "com.apple.compilers.llvm.clang.1_0"
            },
            "conditions": [
                ["OS != 'win'",{
                    "include_dirs": ["<!(node -e \"require('nan')\")"],
                    "link_settings": { "libraries": ["-lsweep"], }
                }],
                ["OS == 'win'",{
                    "include_dirs": [
                        "<!(node -e \"require('nan')\")",
                        "C:/Program Files/sweep/include",
                        "C:/Program Files (x86)/sweep/include"
                    ],
                    "library_dirs": [
                        "C:/Program Files/sweep/lib",
                        "C:/Program Files (x86)/sweep/lib"
                    ],
                    "libraries": [
                        "-lsweep"
                    ]
                }]
            ],
        },
    ]
}
