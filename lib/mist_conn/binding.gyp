{
    "targets": [
        {
            "target_name": "mist_conn",
            "type": "<(library)",
            "cflags_cc": [ "-frtti -fexceptions -std=c++11" ],
            "defines": [
                "BUILDING_MIST_CONN",
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
            "link_settings": {
                "libraries": [
                    "-lnss3",
                    "-lnspr4",
                    "-lplc4",
                    "-lssl3",
                    "-lsmime3",
                    "-lnghttp2",
                ],
            },
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "./include",
                "./src",
            ],
            "sources": [
                "./src/conn/conn.cpp",
                "./src/conn/peer.cpp",
                "./src/conn/service.cpp",
                "./src/crypto/hash.cpp",
                "./src/crypto/key.cpp",
                "./src/crypto/random.cpp",
                "./src/crypto/sha3.c",
                "./src/error/mist.cpp",
                "./src/error/nghttp2.cpp",
                "./src/error/nss.cpp",
                "./src/h2/client_session.cpp",
                "./src/h2/client_stream.cpp",
                "./src/h2/lane.cpp",
                "./src/h2/server_session.cpp",
                "./src/h2/server_stream.cpp",
                "./src/h2/session.cpp",
                "./src/h2/stream.cpp",
                "./src/h2/util.cpp",
                "./src/h2/websocket.cpp",
                "./src/io/address.cpp",
                "./src/io/io_context.cpp",
                "./src/io/rdv_socket.cpp",
                "./src/io/tcp_socket.cpp",
                "./src/io/ssl_context.cpp",
                "./src/io/ssl_socket.cpp",
                "./src/nss/nss_base.cpp",
                "./src/nss/pubkey.cpp",
                "./src/tor/tor.cpp",
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
            'direct_dependent_settings': {
                #'defines': [
                #    'DEFINE_TO_USE_LIBBAR',
                #],
                'include_dirs': [
                    './include',
                ],
            },
            'conditions': [
                [ '"<(library)"=="shared_library"',
                    {
                        "defines+" : [
                            "MIST_CONN_SHARED",
                        ],
                        'direct_dependent_settings+': {
                            "defines+" : [
                                "MIST_CONN_SHARED",
                            ],
                        },
                    }
                ],
                [ 'OS=="mac"',
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
                [ 'OS!="win"', {
                        'variables': {
                        },
                        "include_dirs+": [
                            "/usr/include/nspr",
                            "/usr/include/nss",
                            "/usr/local/include/nspr",
                            "/usr/local/include/nss",
                            "/usr/local/include",
                        ],
                        "libraries+": [
                            "-lpthread",
                            "-lboost_exception",
                            "-lboost_filesystem",
                            "-lboost_random",
                            "-lboost_system",
                        ]
                    }
                ],
                [
                    'OS=="win"', {
                        'defines+': [
                            "_SSIZE_T_",
                            "_SSIZE_T_DEFINED",
                            "__ssize_t_defined",
                            "ssize_t=long",
                        ],
                        'variables': {
                            "BOOST_ROOT": "<!(echo %BOOST_ROOT%)",
                            "BOOST_BUILDSTRING": "-vc140-mt-sgd-1_61",
                            "NGHTTP2_ROOT": "<!(echo %NGHTTP2_ROOT%)",
                            "NSS_ROOT": "<!(echo %NSS_ROOT%)",
                            "NSS_BUILDSTRING": "WIN954.0_64_DBG.OBJ",
                        },
                        "include_dirs+": [
                            ">(BOOST_ROOT)",
                            ">(NGHTTP2_ROOT)/lib/includes",
                            ">(NSS_ROOT)",
                            ">(NSS_ROOT)/dist/public/nss",
                            ">(NSS_ROOT)/dist/>(NSS_BUILDSTRING)/include"
                        ],
                        "libraries+": [
                            "-llibboost_exception>(BOOST_BUILDSTRING)",
                            "-llibboost_filesystem>(BOOST_BUILDSTRING)",
                            "-llibboost_random>(BOOST_BUILDSTRING)",
                            "-llibboost_system>(BOOST_BUILDSTRING)"
                        ],
                        'msvs_settings': {
                            "VCCLCompilerTool": {
                                "AdditionalOptions": [
                                    "/EHsc"
                                ]
                            },
                            'VCLinkerTool': {
                                'AdditionalLibraryDirectories': [
                                    ">(BOOST_ROOT)/stage/lib",
                                    ">(NGHTTP2_ROOT)/lib/MSVC_obj",
                                    ">(NSS_ROOT)/dist/>(NSS_BUILDSTRING)/lib"
                                ]
                            }
                        }
                    }
                ]
            ],
        },
    ],
}
