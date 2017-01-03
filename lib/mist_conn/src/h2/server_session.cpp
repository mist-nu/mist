#include <cassert>
#include <cstddef>
#include <list>
#include <map>
#include <memory>

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include "io/socket.hpp"

#include "error/nghttp2.hpp"
#include "error/mist.hpp"

#include "memory/nghttp2.hpp"

#include "h2/session_impl.hpp"
#include "h2/server_session.hpp"
#include "h2/server_session_impl.hpp"
#include "h2/server_stream_impl.hpp"

namespace mist
{
namespace h2
{

/*
 * ServerSession
 */
ServerSession::ServerSession(std::shared_ptr<io::Socket> socket)
  : _impl(std::make_shared<ServerSessionImpl>(socket))
{
    // TODO:
    start();
}

ServerSession::ServerSession(std::shared_ptr<ServerSessionImpl> impl)
  : _impl(impl)
{}

void
ServerSession::setOnRequest(server_request_callback cb)
{
  _impl->setOnRequest(std::move(cb));
}

void
ServerSession::shutdown()
{
  _impl->shutdown();
}

void
ServerSession::start()
{
    _impl->start();
}

void
ServerSession::stop()
{
    _impl->stop();
}

bool
ServerSession::isStopped() const
{
  return _impl->isStopped();
}

void
ServerSession::setOnError(error_callback cb)
{
  _impl->setOnError(std::move(cb));
}

/*
 * ServerSessionImpl
 */
ServerSessionImpl::ServerSessionImpl(std::shared_ptr<io::Socket> socket)
  : SessionImpl(std::move(socket), true)
{
}

void
ServerSessionImpl::setOnRequest(server_request_callback cb)
{
  _onRequest = std::move(cb);
}

int
ServerSessionImpl::onBeginHeaders(const nghttp2_frame* frame)
{
  if (frame->hd.type == NGHTTP2_HEADERS
      && frame->headers.cat == NGHTTP2_HCAT_REQUEST) {
    /* Client created a stream */
    auto strm = std::make_shared<ServerStreamImpl>(
      shared_from_base<ServerSessionImpl>());
    strm->setStreamId(frame->hd.stream_id);
    insertStream(std::move(strm));
  }
  return 0;
}

int
ServerSessionImpl::onHeader(const nghttp2_frame* frame,
  const std::uint8_t* name, std::size_t namelen, const std::uint8_t* value,
  std::size_t valuelen, std::uint8_t flags)
{
  auto stream = findStream<ServerStreamImpl>(frame->hd.stream_id);
  if (!stream)
    return 0;
  return (*stream)->onHeader(frame, name, namelen, value, valuelen, flags);
}

int
ServerSessionImpl::onFrameSend(const nghttp2_frame* frame)
{
  auto stream = findStream<ServerStreamImpl>(frame->hd.stream_id);
  if (!stream)
    return 0;
  return (*stream)->onFrameSend(frame);
}

int
ServerSessionImpl::onFrameNotSend(const nghttp2_frame* frame, int error_code)
{
  auto stream = findStream<ServerStreamImpl>(frame->hd.stream_id);
  if (!stream)
    return 0;
  return (*stream)->onFrameNotSend(frame, error_code);
}

int
ServerSessionImpl::onFrameRecv(const nghttp2_frame* frame)
{
  auto stream = findStream<ServerStreamImpl>(frame->hd.stream_id);
  if (!stream)
    return 0;
  if (frame->hd.type == NGHTTP2_HEADERS
      && frame->headers.cat == NGHTTP2_HCAT_REQUEST) {
    /* TODO: Check headers for WebSocket / alternative services */
    if (_onRequest)
      _onRequest(ServerRequest(*stream));
  }
  return (*stream)->onFrameRecv(frame);
}

int
ServerSessionImpl::onDataChunkRecv(std::uint8_t flags, std::int32_t stream_id,
  const std::uint8_t* data, std::size_t length)
{
  auto stream = findStream<ServerStreamImpl>(stream_id);
  if (!stream)
    return 0;
  return (*stream)->onDataChunkRecv(flags, data, length);
}

int
ServerSessionImpl::onStreamClose(std::int32_t stream_id,
  std::uint32_t error_code)
{
  auto stream = findStream<ServerStreamImpl>(stream_id);
  if (!stream)
    return 0;
  return (*stream)->onStreamClose(error_code);
}

ssize_t
ServerSessionImpl::onRead(std::int32_t stream_id, std::uint8_t* data,
  std::size_t length, std::uint32_t* flags)
{
  auto stream = findStream<ServerStreamImpl>(stream_id);
  if (!stream) {
    *flags |= NGHTTP2_ERR_EOF;
    return 0;
  }
  return (*stream)->onRead(data, length, flags);
}

} // namespace h2
} // namespace mist
