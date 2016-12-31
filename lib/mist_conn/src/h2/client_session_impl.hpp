#ifndef __MIST_SRC_H2_CLIENT_SESSION_IMPL_HPP__
#define __MIST_SRC_H2_CLIENT_SESSION_IMPL_HPP__

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

#include "io/ssl_socket.hpp"

#include "memory/nghttp2.hpp"

#include "h2/types.hpp"
#include "h2/session.hpp"
#include "h2/session_impl.hpp"
#include "h2/client_session.hpp"

namespace mist
{
namespace h2
{

class ClientStreamImpl;

/*
 * ClientSessionImpl
 */
class ClientSessionImpl : public SessionImpl
{
public:

  ClientSessionImpl(std::shared_ptr<io::Socket> socket);

  /* Create a new stream by submitting */
  std::shared_ptr<ClientStreamImpl>
  submit(std::string method, std::string path, std::string scheme,
    std::string authority, header_map headers,
    generator_callback cb = nullptr);

private:

  /* Disable copy constructor and copy assign */
  ClientSessionImpl(ClientSessionImpl&) = delete;
  ClientSessionImpl& operator=(ClientSessionImpl&) = delete;

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
