/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <boost/system/error_code.hpp>

namespace mist
{

typedef enum {

  MIST_ERR_ASSERTION = 8000,
  MIST_ERR_NOT_HTTP2 = 8001,
  MIST_ERR_NO_KEY_OR_CERT = 8002,
  MIST_ERR_INVALID_KEY_OR_CERT = 8003,
  MIST_ERR_SOCKS_HANDSHAKE = 8004,
  MIST_ERR_PORT_TAKEN = 8005,
  MIST_ERR_DIRECT_LISTEN_PORT_TAKEN = 8006,

  MIST_ERR_TOR_GENERAL_FAILURE = 9000,
  MIST_ERR_TOR_LISTEN_PORT_TAKEN = 9001,
  MIST_ERR_TOR_SOCKS_PORT_TAKEN = 9002,
  MIST_ERR_TOR_CONTROL_PORT_TAKEN = 9003,
  MIST_ERR_TOR_EXECUTION = 9004,
  MIST_ERR_TOR_LOG_FILE = 9005,

} mist_error;

MistConnApi const boost::system::error_category &mist_category() noexcept;

/* Creates a boost::system::error_code with the given error value */
MistConnApi boost::system::error_code make_mist_error(mist_error ev);

}
