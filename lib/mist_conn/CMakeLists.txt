cmake_minimum_required(VERSION 2.8.12)

set(PROJECT_NAME MistConn)
project(${PROJECT_NAME})

add_definitions(-DBUILDING_MIST_CONN)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2 -ggdb -pedantic -frtti -fexceptions -Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHsc -W3")
endif(CMAKE_COMPILER_IS_GNUCXX)

set(PROJECT_ROOT .)
set(PROJECT_LIBS "")

##############
# Boost
##############
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
find_package(Boost 1.50.0 REQUIRED COMPONENTS system filesystem random)
set(PROJECT_LIBS ${PROJECT_LIBS} ${Boost_LIBRARIES})
include_directories(${Boost_INCLUDE_DIRS})

##############
# nghttp2
##############
if(MSVC)
    add_definitions(-D_SSIZE_T_)
    add_definitions(-D_SSIZE_T_DEFINED)
    add_definitions(-D__ssize_t_defined)
    add_definitions(-Dssize_t=long)
endif()
if(NOT DEFINED NGHTTP2_ROOT)
    if(DEFINED ENV{NGHTTP2_ROOT})
        set(NGHTTP2_ROOT $ENV{NGHTTP2_ROOT})
    endif()
endif()
if(DEFINED NGHTTP2_ROOT)
    # TODO: Find out if we can chain together nghttp2's CMakeLists.txt
    #add_subdirectory(${NGHTTP2_ROOT} "${CMAKE_CURRENT_BINARY_DIR}/nghttp2_build")
    include_directories(${NGHTTP2_ROOT}/lib/includes)
    if(MSVC)
        set(PROJECT_LIBS ${PROJECT_LIBS}
            ${NGHTTP2_ROOT}/lib/MSVC_obj/nghttp2.lib
        )
    else()
        # TODO: On linux
        set(PROJECT_LIBS ${PROJECT_LIBS} nghttp2)
    endif()
else()
    set(PROJECT_LIBS ${PROJECT_LIBS} nghttp2)
endif()

##############
# nss/nspr
##############
if(NOT DEFINED NSS_ROOT)
    if(DEFINED ENV{NSS_ROOT})
        set(NSS_ROOT $ENV{NSS_ROOT})
    endif()
endif()
if(DEFINED NSS_ROOT)
    if(MSVC)
        set(NSS_OS_CONFIG "WIN954.0")
    else()
        set(NSS_OS_CONFIG "Linux")
    endif()
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(NSS_64BIT_TAG "_64")
    else()
        set(NSS_64BIT_TAG "")
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(NSS_BUILDTYPE_TAG "${64BIT_TAG}_DBG")
    else()
        set(NSS_BUILDTYPE_TAG "${64BIT_TAG}_OPT")
    endif()
    set(NSS_BUILDSTRING "${NSS_OS_CONFIG}${NSS_64BIT_TAG}${NSS_BUILDTYPE_TAG}.OBJ")
    include_directories(${NSS_ROOT})
    include_directories(${NSS_ROOT}/dist/public/nss)
    include_directories(${NSS_ROOT}/dist/${NSS_BUILDSTRING}/include)
    if(MSVC)
        set(PROJECT_LIBS ${PROJECT_LIBS}
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/nss3.lib
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/nspr4.lib
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/plc4.lib
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/ssl3.lib
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/smime3.lib

            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/sectool.lib
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/nssutil.lib
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/plds4.lib
        )
    else()
        set(PROJECT_LIBS ${PROJECT_LIBS}
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/libnss3
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/libnspr4
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/libplc4
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/libssl3
            ${NSS_ROOT}/dist/${NSS_BUILDSTRING}/lib/libsmime3
        )
    endif()
else()
    if(MSVC)
    else()
        include_directories(/usr/include/nss)
        include_directories(/usr/include/nspr)
        include_directories(/usr/local/include/nss)
        include_directories(/usr/local/include/nspr)
    endif()
    set(PROJECT_LIBS ${PROJECT_LIBS} nss3 nspr4 plc4 ssl3 smime3)
endif()

################################
# Normal Libraries & Executables
################################

set(MIST_CONN_SOURCE ${PROJECT_ROOT}/src)
include_directories(${PROJECT_ROOT}/include)
include_directories(${PROJECT_ROOT}/src)

add_library(mist_conn STATIC 
    ${MIST_CONN_SOURCE}/conn/conn.cpp
    ${MIST_CONN_SOURCE}/conn/peer.cpp
    ${MIST_CONN_SOURCE}/conn/service.cpp
    ${MIST_CONN_SOURCE}/crypto/hash.cpp
    ${MIST_CONN_SOURCE}/crypto/key.cpp
    ${MIST_CONN_SOURCE}/crypto/random.cpp
    ${MIST_CONN_SOURCE}/crypto/sha3.c
    ${MIST_CONN_SOURCE}/error/mist.cpp
    ${MIST_CONN_SOURCE}/error/nghttp2.cpp
    ${MIST_CONN_SOURCE}/error/nss.cpp
    ${MIST_CONN_SOURCE}/h2/client_session.cpp
    ${MIST_CONN_SOURCE}/h2/client_stream.cpp
    ${MIST_CONN_SOURCE}/h2/lane.cpp
    ${MIST_CONN_SOURCE}/h2/server_session.cpp
    ${MIST_CONN_SOURCE}/h2/server_stream.cpp
    ${MIST_CONN_SOURCE}/h2/session.cpp
    ${MIST_CONN_SOURCE}/h2/stream.cpp
    ${MIST_CONN_SOURCE}/h2/util.cpp
    ${MIST_CONN_SOURCE}/h2/websocket.cpp
    ${MIST_CONN_SOURCE}/io/address.cpp
    ${MIST_CONN_SOURCE}/io/io_context.cpp
    ${MIST_CONN_SOURCE}/io/rdv_socket.cpp
    ${MIST_CONN_SOURCE}/io/ssl_context.cpp
    ${MIST_CONN_SOURCE}/io/ssl_socket.cpp
    ${MIST_CONN_SOURCE}/io/tcp_socket.cpp
    ${MIST_CONN_SOURCE}/nss/nss_base.cpp
    ${MIST_CONN_SOURCE}/nss/pubkey.cpp
    #${MIST_CONN_SOURCE}/node/async.cpp
    #${MIST_CONN_SOURCE}/node/module.cpp
    ${MIST_CONN_SOURCE}/tor/tor.cpp
)

target_link_libraries(mist_conn ${PROJECT_LIBS})
