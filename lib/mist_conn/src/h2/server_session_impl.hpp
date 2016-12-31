#ifndef __MIST_SRC_H2_SERVER_SESSION_IMPL_HPP__
#define __MIST_SRC_H2_SERVER_SESSION_IMPL_HPP__

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <list>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <nghttp2/nghttp2.h>

#include "error/nghttp2.hpp"
#include "error/mist.hpp"

#include "io/socket.hpp"
#include "io/ssl_socket.hpp"

#include "memory/nghttp2.hpp"

#include "h2/types.hpp"
#include "h2/session_impl.hpp"

namespace mist
{
namespace h2
{

class ServerSessionImpl : public SessionImpl
{
public:

  ServerSessionImpl(std::shared_ptr<io::Socket> socket);

  /* Set the on request callback */
  void setOnRequest(server_request_callback cb);

private:

  /* Disable copy constructor and copy assign */
  ServerSessionImpl(ServerSessionImpl&) = delete;
  ServerSessionImpl& operator=(ServerSessionImpl&) = delete;

  server_request_callback _onRequest;

protected:

  /* nghttp2 callbacks */
  virtual int onBeginHeaders(const nghttp2_frame* frame) override;

  virtual int onHeader(const nghttp2_frame* frame, const std::uint8_t* name,
    std::size_t namelen, const std::uint8_t* value, std::size_t valuelen,
    std::uint8_t flags) override;

  virtual int onFrameSend(const nghttp2_frame* frame) override;

  virtual int onFrameNotSend(const nghttp2_frame* frame,
    int errorCode) override;

  virtual int onFrameRecv(const nghttp2_frame* frame) override;

  virtual int onDataChunkRecv(std::uint8_t flags, std::int32_t stream_id,
    const std::uint8_t* data, std::size_t len) override;

  virtual int onStreamClose(std::int32_t stream_id,
    std::uint32_t error_code) override;

};

} // namespace h2
} // namespace mist

#endif
