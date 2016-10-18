{
    "targets": [
        {
            "include_dirs": ["<!(node -e \"require('nan')\")"],
            "target_name": "sweejs",
            "sources": [
                "sweejs.cc"
            ],
            "conditions": [
                ["OS != 'win'",{
                    "cflags_cc": ["-Wall -Wextra -pedantic -std=c++11"],
                    "cflags_cc!": ["-fno-exceptions", "-fno-rtti"],
                    "libraries": ["-lsweep"],
                }]
            ],
        },
    ],
}
