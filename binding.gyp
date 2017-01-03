{
    "targets": [
        {
            "target_name": "mist",
            "type": "loadable_module",
            "defines" : [
                "CHANGE_G3LOG_DEBUG_TO_DBUG"
            ],
            "sources": [ 
                "src/Node/Async.cpp",
                "src/Node/CentralWrap.cpp",
                "src/Node/ClientStreamWrap.cpp",
                "src/Node/DatabaseWrap.cpp",
                "src/Node/Module.cpp",
                "src/Node/ServerStreamWrap.cpp",
                "src/Node/ServiceWrap.cpp",
                "src/Node/PeerWrap.cpp",
                "src/Node/SHA3Wrap.cpp",
                "src/Node/SHA3HasherWrap.cpp",
                "src/Node/PublicKeyWrap.cpp",
                "src/Node/PrivateKeyWrap.cpp",
                "src/Node/SignatureWrap.cpp",
                "src/Node/TransactionWrap.cpp",
                "src/Node/Wrap.cpp",
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
            "dependencies": [
                "libmist",
                "_mist_conn",
            ],
            "include_dirs" : [
                "<!(node -e \"require('nan')\")",
                "include",
                "lib",
                "lib/g3log/src",
                "lib/mist_conn/include",
                "lib/sqlite3",
                "lib/sqlitecpp/include"
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
                            "-lboost_filesystem",
                            "-lboost_system",
                            "-lg3logger",
                            "-L<(module_root_dir)/lib/g3log/build",
                        ]
                    }
                ],
                [
                    'OS=="win"', {
                        'variables': {
                            "BOOST_ROOT": "<!(echo %BOOST_ROOT%)",
                            "BOOST_BUILDSTRING": "-vc140-mt-sgd-1_61",
                            "NGHTTP2_ROOT": "<!(echo %NGHTTP2_ROOT%)",
                            "NSS_ROOT": "<!(echo %NSS_ROOT%)",
                            "NSS_BUILDSTRING": "WIN954.0_64_DBG.OBJ"
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
                            "-llibboost_system>(BOOST_BUILDSTRING)",
                            "-lg3logger",
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
                                    ">(NSS_ROOT)/dist/>(NSS_BUILDSTRING)/lib",
                                    "<(module_root_dir)/lib/g3log/build/Debug",
                                ]
                            }
                        }
                    }
                ]
            ],
        },
        {
            "target_name": "libmist",
            "type": "<(library)",
            "defines" : [
                "CHANGE_G3LOG_DEBUG_TO_DBUG"
            ],
            "dependencies": [
                "libs",
                "_mist_conn",
            ],
            "include_dirs": [
                "include",
                "lib",
                "lib/g3log/src",
                "lib/mist_conn/include",
                "lib/sqlite3",
                "lib/sqlitecpp/include"
            ],
            "sources": [
                "src/Central.cpp",
                "src/CryptoHelper.cpp",
                "src/Database.cpp",
                "src/Error.cpp",
                "src/Exception.cpp",
                "src/ExchangeFormat.cpp",
                "src/Helper.cpp",
                "src/JSONstream.cpp",
                "src/PrivateUserAccount.cpp",
                "src/Query.cpp",
                "src/RemoteTransaction.cpp",
                "src/Transaction.cpp",
                "src/UserAccount.cpp"
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
                            "-lboost_filesystem",
                            "-lboost_system",
                            "-lg3logger",
                            "-L<(module_root_dir)/lib/g3log/build",
                        ]
                    }
                ],
                [ 'OS=="win"',
                    {
                        'variables': {
                            "BOOST_ROOT": "<!(echo %BOOST_ROOT%)",
                            "BOOST_BUILDSTRING": "-vc140-mt-sgd-1_61",
                            "NGHTTP2_ROOT": "<!(echo %NGHTTP2_ROOT%)",
                            "NSS_ROOT": "<!(echo %NSS_ROOT%)",
                            "NSS_BUILDSTRING": "WIN954.0_64_DBG.OBJ"
                        },
                        "include_dirs+": [
                            ">(BOOST_ROOT)"
                        ],
                        "libraries+": [
                            "-llibboost_filesystem>(BOOST_BUILDSTRING)",
                            "-llibboost_random>(BOOST_BUILDSTRING)",
                            "-lg3logger",
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
                                    ">(NSS_ROOT)/dist/>(NSS_BUILDSTRING)/lib",
                                    "<(module_root_dir)/lib/g3log/build/Debug",
                                ]
                            }
                        }
                    }
                ]
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
                ],
            }
        },
        {
            "target_name": "libs",
            "type": "<(library)",
            "defines" : [
                "DSQLITE_OMIT_LOAD_EXTENSION"
            ],
            "include_dirs": [
                "lib/sqlite3",
                "lib/sqlitecpp/include"
            ],
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
            "link_settings": {
                "libraries": [
                ],
            }
        },
        {
            "target_name": "_mist_conn",
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
                "lib/mist_conn/include",
                "lib/mist_conn/src",
            ],
            "sources": [
                "lib/mist_conn/src/conn/conn.cpp",
                "lib/mist_conn/src/conn/peer.cpp",
                "lib/mist_conn/src/conn/service.cpp",
                "lib/mist_conn/src/crypto/hash.cpp",
                "lib/mist_conn/src/crypto/key.cpp",
                "lib/mist_conn/src/crypto/random.cpp",
                "lib/mist_conn/src/crypto/sha3.c",
                "lib/mist_conn/src/error/mist.cpp",
                "lib/mist_conn/src/error/nghttp2.cpp",
                "lib/mist_conn/src/error/nss.cpp",
                "lib/mist_conn/src/h2/client_session.cpp",
                "lib/mist_conn/src/h2/client_stream.cpp",
                "lib/mist_conn/src/h2/lane.cpp",
                "lib/mist_conn/src/h2/server_session.cpp",
                "lib/mist_conn/src/h2/server_stream.cpp",
                "lib/mist_conn/src/h2/session.cpp",
                "lib/mist_conn/src/h2/stream.cpp",
                "lib/mist_conn/src/h2/util.cpp",
                "lib/mist_conn/src/h2/websocket.cpp",
                "lib/mist_conn/src/io/address.cpp",
                "lib/mist_conn/src/io/io_context.cpp",
                "lib/mist_conn/src/io/rdv_socket.cpp",
                "lib/mist_conn/src/io/tcp_socket.cpp",
                "lib/mist_conn/src/io/ssl_context.cpp",
                "lib/mist_conn/src/io/ssl_socket.cpp",
                "lib/mist_conn/src/nss/nss_base.cpp",
                "lib/mist_conn/src/nss/pubkey.cpp",
                "lib/mist_conn/src/tor/tor.cpp",
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
            'conditions': [
                [ '"<(library)"=="shared_library"',
                    {
                        "defines+" : [
                            "MIST_CONN_SHARED",
                        ],
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
                            "NSS_BUILDSTRING": "WIN954.0_64_DBG.OBJ"
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
            ]
        }
    ],
}

