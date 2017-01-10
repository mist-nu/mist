/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <boost/system/error_code.hpp>
#include "error/mist.hpp"

namespace mist
{

namespace
{

class mist_error_category : public boost::system::error_category
{
  const char *name() const noexcept
  {
    return "nghttp2";
  }
  
  std::string message(int ev) const
  {
    switch (ev) {
    case MIST_ERR_ASSERTION:
      return "MIST_ERR_ASSERTION";
    case MIST_ERR_NOT_HTTP2:
      return "MIST_ERR_NOT_HTTP2";
    case MIST_ERR_NO_KEY_OR_CERT:
      return "MIST_ERR_NO_KEY_OR_CERT";
    case MIST_ERR_INVALID_KEY_OR_CERT:
      return "MIST_ERR_INVALID_KEY_OR_CERT";
    case MIST_ERR_SOCKS_HANDSHAKE:
      return "MIST_ERR_SOCKS_HANDSHAKE";
    case MIST_ERR_PORT_TAKEN:
      return "MIST_ERR_PORT_TAKEN";
    case MIST_ERR_DIRECT_LISTEN_PORT_TAKEN:
      return "MIST_ERR_DIRECT_LISTEN_PORT_TAKEN";
    case MIST_ERR_TOR_GENERAL_FAILURE:
      return "MIST_ERR_TOR_GENERAL_FAILURE";
    case MIST_ERR_TOR_LISTEN_PORT_TAKEN:
      return "MIST_ERR_TOR_LISTEN_PORT_TAKEN";
    case MIST_ERR_TOR_SOCKS_PORT_TAKEN:
      return "MIST_ERR_TOR_SOCKS_PORT_TAKEN";
    case MIST_ERR_TOR_CONTROL_PORT_TAKEN:
      return "MIST_ERR_TOR_CONTROL_PORT_TAKEN";
    case MIST_ERR_TOR_EXECUTION:
      return "MIST_ERR_TOR_EXECUTION";
    case MIST_ERR_TOR_LOG_FILE:
      return "MIST_ERR_TOR_LOG_FILE";
    default:
      return "Unknown mist error " + std::to_string(ev);
    }
  }
} instance;

} // namespace

MistConnApi
const boost::system::error_category &mist_category() noexcept
{
  return instance;
}

MistConnApi
boost::system::error_code make_mist_error(mist_error ev)
{
  return boost::system::error_code(ev, mist_category());
}

}
