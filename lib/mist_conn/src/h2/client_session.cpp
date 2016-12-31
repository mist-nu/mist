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

#include "h2/client_session.hpp"
#include "h2/client_session_impl.hpp"
#include "h2/client_stream_impl.hpp"

namespace mist
{
namespace h2
{

/*
 * ClientSession
 */
ClientSession::ClientSession(std::shared_ptr<io::Socket> socket)
  : _impl(std::make_shared<ClientSessionImpl>(socket))
{
    // TODO:
    start();
}

ClientSession::ClientSession(std::shared_ptr<ClientSessionImpl> impl)
  : _impl(impl)
{}

ClientRequest
ClientSession::submitRequest(std::string method, std::string path,
  std::string scheme, std::string authority, header_map headers,
  generator_callback cb)
{
  std::shared_ptr<ClientStreamImpl> strm = _impl->submit(
    std::move(method), std::move(path), std::move(scheme),
    std::move(authority), std::move(headers), std::move(cb));
  return ClientRequest(strm);
}

void
ClientSession::shutdown()
{
  _impl->shutdown();
}

void
ClientSession::start()
{
    _impl->start();
}

void
ClientSession::stop()
{
    _impl->stop();
}

bool
ClientSession::isStopped() const
{
  return _impl->isStopped();
}

void
ClientSession::setOnError(error_callback cb)
{
  _impl->setOnError(std::move(cb));
}

/*
 * ClientSessionImpl
 */

ClientSessionImpl::ClientSessionImpl(std::shared_ptr<io::Socket> socket)
  : SessionImpl(std::move(socket), false)
{
}

std::shared_ptr<ClientStreamImpl>
ClientSessionImpl::submit(std::string method, std::string path,
  std::string scheme, std::string authority, header_map headers,
  generator_callback cb)
{
  auto strm = std::make_shared<ClientStreamImpl>(
    shared_from_base<ClientSessionImpl>());
  boost::system::error_code ec = strm->submit(std::move(method),
    std::move(path), std::move(scheme), std::move(authority),
    std::move(headers), std::move(cb));
  insertStream(strm);
  write();
  return strm;
}

int
ClientSessionImpl::onBeginHeaders(const nghttp2_frame* frame)
{
  if (frame->hd.type == NGHTTP2_PUSH_PROMISE) {
    /* Server created a push stream */
    auto strm = std::make_shared<ClientStreamImpl>(
      shared_from_base<ClientSessionImpl>());
    strm->setStreamId(frame->push_promise.promised_stream_id);
    insertStream(std::move(strm));
  }
  return 0;
}

int
ClientSessionImpl::onHeader(const nghttp2_frame* frame,
  const std::uint8_t* name, std::size_t namelen, const std::uint8_t* value,
  std::size_t valuelen, std::uint8_t flags)
{
  auto stream = findStream<ClientStreamImpl>(frame->hd.stream_id);
  if (!stream)
    return 0;
  return (*stream)->onHeader(frame, name, namelen, value, valuelen, flags);
}

int
ClientSessionImpl::onFrameSend(const nghttp2_frame* frame)
{
  auto stream = findStream<ClientStreamImpl>(frame->hd.stream_id);
  if (!stream)
    return 0;
  return (*stream)->onFrameSend(frame);
}

int
ClientSessionImpl::onFrameNotSend(const nghttp2_frame* frame, int errorCode)
{
  auto stream = findStream<ClientStreamImpl>(frame->hd.stream_id);
  if (!stream)
    return 0;
  return (*stream)->onFrameNotSend(frame, errorCode);
}

int
ClientSessionImpl::onFrameRecv(const nghttp2_frame* frame)
{
  auto stream = findStream<ClientStreamImpl>(frame->hd.stream_id);
  if (!stream)
    return 0;
  if (frame->hd.type == NGHTTP2_PUSH_PROMISE) {
    auto pushStream = findStream<ClientStreamImpl>(
      frame->push_promise.promised_stream_id);
    if (pushStream) {
      (*stream)->onPush(*pushStream);
    }
  }
  return (*stream)->onFrameRecv(frame);
}

int
ClientSessionImpl::onDataChunkRecv(std::uint8_t flags, std::int32_t stream_id,
  const std::uint8_t* data, std::size_t length)
{
  auto stream = findStream<ClientStreamImpl>(stream_id);
  if (!stream)
    return 0;
  return (*stream)->onDataChunkRecv(flags, data, length);
}

int
ClientSessionImpl::onStreamClose(std::int32_t stream_id,
  std::uint32_t error_code)
{
  auto stream = findStream<ClientStreamImpl>(stream_id);
  if (!stream)
    return 0;
  return (*stream)->onStreamClose(error_code);
}

} // namespace h2
} // namespace mist
