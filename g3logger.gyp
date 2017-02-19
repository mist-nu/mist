{
    "targets": [
        {
            "target_name": "g3logger",
            "type": "static_library",
            "defines": [
                "CHANGE_G3LOG_DEBUG_TO_DBUG",
            ],
            "cflags_cc": [
                "-fexceptions",
                "-frtti",
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
            "include_dirs": [
                "./lib/g3log/src",
                "./lib/g3log/src/g3log",
            ],
            "sources": [
                "./lib/g3log/src/filesink.cpp",
                "./lib/g3log/src/g3log.cpp",
                "./lib/g3log/src/logcapture.cpp",
                "./lib/g3log/src/loglevels.cpp",
                "./lib/g3log/src/logmessage.cpp",
                "./lib/g3log/src/logworker.cpp",
                "./lib/g3log/src/time.cpp",
            ],
            'direct_dependent_settings': {
                'defines': [
                    'CHANGE_G3LOG_DEBUG_TO_DBUG',
                ],
                'include_dirs': [
                    './lib/g3log/src',
                ],
            },
            'conditions': [
                [ 'OS=="mac"', {
                    "xcode_settings": {
                        "MACOSX_DEPLOYMENT_TARGET": "10.9",
                        "CLANG_CXX_LIBRARY": "libc++",
                        "GCC_ENABLE_CPP_RTTI": "YES",
                        "GCC_ENABLE_CPP_EXCEPTIONS": "YES"
                    },
		    'defines': [
		      'thread_local=',
		      'TIME_UTC=1'
		    ]
                } ],
                [ 'OS!="win"', {
                    "sources+": [
                        "./lib/g3log/src/crashhandler_unix.cpp",
                    ],
                } ],
                [ 'OS=="win"', {
                    "sources+": [
                        "./lib/g3log/src/crashhandler_windows.cpp",
                        "./lib/g3log/src/stacktrace_windows.cpp",
                    ],
                } ]
            ],
        },
    ],
}
