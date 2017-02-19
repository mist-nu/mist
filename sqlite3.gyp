{
    "targets": [
        {
            "target_name": "sqlite3",
            "type": "<(library)",
            "defines" : [
                "DSQLITE_OMIT_LOAD_EXTENSION"
            ],
            "include_dirs": [
                "lib/sqlite3",
                "lib/sqlitecpp/include",
            ],
            "direct_dependent_settings": {
                "include_dirs": [
                    "./lib/sqlite3",
                    "./lib/sqlitecpp/include",
                ],
            },
            "sources": [
                "lib/sqlite3/sqlite3.c",
                "lib/sqlitecpp/src/Backup.cpp",
                "lib/sqlitecpp/src/Column.cpp",
                "lib/sqlitecpp/src/Database.cpp",
                "lib/sqlitecpp/src/Exception.cpp",
                "lib/sqlitecpp/src/Statement.cpp",
                "lib/sqlitecpp/src/Transaction.cpp",
            ],
            "conditions": [
                [ "OS==\"mac\"",
                    {
                        "xcode_settings": {
                            "MACOSX_DEPLOYMENT_TARGET": "10.9",
                            "CLANG_CXX_LIBRARY": "libc++",
                            "GCC_ENABLE_CPP_RTTI": "YES",
                            "GCC_ENABLE_CPP_EXCEPTIONS": "YES"
                        },
                        "libraries+": [
                            "-L/usr/local/lib"
                        ],
                        "include_dirs+": [
                            "/usr/local/include"
                        ]
                    }
                ],
                [ 'OS!="win"',
                    {
                        "libraries+": [
                            "-ldl",
                            "-lm",
                            "-lpthread",
                        ]
                    }
                ],
            ],
            "cflags_cc": [
                "-fexceptions",
                "-std=c++11",
            ],
            "cflags!": [ 
                "-fno-exceptions",
                "-fno-rtti",
                "-std=gnu++0x"
            ],
            "cflags_cc!": [ 
                "-fno-exceptions",
                "-fno-rtti",
                "-std=gnu++0x"
            ],
        },
    ],
}
