{
    "targets": [
        {
            "target_name": "mist",
            "type": "loadable_module",
            #"defines" : [
            #    "CHANGE_G3LOG_DEBUG_TO_DBUG"
            #],
            "sources": [ 
                "src/Node/Async.cpp",
                "src/Node/CentralWrap.cpp",
                "src/Node/ClientStreamWrap.cpp",
                "src/Node/DatabaseWrap.cpp",
                "src/Node/Module.cpp",
                "src/Node/ServerStreamWrap.cpp",
                "src/Node/ServiceWrap.cpp",
                "src/Node/SocketWrap.cpp",
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
                "g3logger.gyp:g3logger",
                "lib/mist_conn/binding.gyp:mist_conn",
            ],
            "include_dirs" : [
                "<!(node -e \"require('nan')\")",
                "include",
                #"lib",
                #"lib/g3log/src",
                #"lib/mist_conn/include",
                #"lib/sqlite3",
                #"lib/sqlitecpp/include"
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
                            #"-lg3logger",
                            #"-L<(module_root_dir)/lib/g3log/build",
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
                            #"-lg3logger",
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
                                    #"<(module_root_dir)/lib/g3log/build/Debug",
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
            #"defines" : [
            #    "CHANGE_G3LOG_DEBUG_TO_DBUG"
            #],
            "dependencies": [
                "sqlite3.gyp:sqlite3",
                "g3logger.gyp:g3logger",
                "lib/mist_conn/binding.gyp:mist_conn",
            ],
            "export_dependent_settings": [
                "sqlite3.gyp:sqlite3",
            ],
            "include_dirs": [
                "include",
                #"lib",
                #"lib/g3log/src",
                #"lib/mist_conn/include",
                #"lib/sqlite3",
                #"lib/sqlitecpp/include"
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
                            #"-lg3logger",
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
                            #"-lg3logger",
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
                                    #"<(module_root_dir)/lib/g3log/build/Debug",
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
        },
    ],
}
