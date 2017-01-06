#include <cstddef>
#include <memory>
#include <string>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <nghttp2/nghttp2.h>

#include "h2/session.hpp"
#include "h2/server_session.hpp"
#include "h2/server_stream_impl.hpp"
#include "h2/stream.hpp"

namespace mist
{
namespace h2
{

/*
* ServerRequest
*/
MistConnApi
ServerRequest::ServerRequest(std::shared_ptr<ServerStreamImpl> impl)
  : _impl(impl)
{}

MistConnApi
ServerStream
ServerRequest::stream()
{
  return ServerStream(_impl);
}

MistConnApi
const header_map&
ServerRequest::headers() const
{
  return _impl->request().headers();
}

MistConnApi
const boost::optional<std::uint64_t>&
ServerRequest::contentLength() const
{
  return _impl->request().contentLength();
}

MistConnApi
const boost::optional<std::string>&
ServerRequest::method() const
{
  return _impl->request().method();
}

MistConnApi
const boost::optional<std::string>&
ServerRequest::path() const
{
  return _impl->request().path();
}

MistConnApi
const boost::optional<std::string>&
ServerRequest::scheme() const
{
  return _impl->request().scheme();
}

MistConnApi
const boost::optional<std::string>&
ServerRequest::authority() const
{
  return _impl->request().authority();
}

MistConnApi
void
ServerRequest::setOnData(data_callback cb)
{
  _impl->request().setOnData(std::move(cb));
}

/*
* ServerRequestImpl
*/
ServerRequestImpl::ServerRequestImpl(ServerStreamImpl & stream)
  : _stream(stream)
{
}

void
ServerRequestImpl::setOnData(data_callback cb)
{
  auto anchor(_stream.shared_from_this());
  _onData = [anchor, cb](const std::uint8_t* data, std::size_t length) {
    cb(data, length);
  };
}

void
ServerRequestImpl::onData(const std::uint8_t* data, std::size_t length)
{
  if (_onData) {
    /* We clear the onData callback on EOF */
    _onData(data, length);
    if (length == 0)
      _onData = nullptr;
  }
}

/*
* ServerResponse
*/
MistConnApi
ServerResponse::ServerResponse(std::shared_ptr<ServerStreamImpl> impl)
  : _impl(impl)
{}

MistConnApi
ServerStream
ServerResponse::stream()
{
  return ServerStream(_impl);
}

MistConnApi
void
ServerResponse::setOnRead(generator_callback cb)
{
  _impl->setOnRead(std::move(cb));
}

MistConnApi
void
ServerResponse::end()
{
  _impl->end();
}

MistConnApi
void
ServerResponse::end(const std::string& buffer)
{
  _impl->end(buffer);
}

MistConnApi
void
ServerResponse::end(const std::vector<std::uint8_t>& buffer)
{
  _impl->end(buffer);
}

MistConnApi
boost::system::error_code
ServerResponse::submitTrailers(header_map headers)
{
  return _impl->submitTrailers(std::move(headers));
}

MistConnApi
const header_map&
ServerResponse::headers() const
{
  return _impl->response().headers();
}

MistConnApi
const boost::optional<std::uint64_t>&
ServerResponse::contentLength() const
{
  return _impl->response().contentLength();
}

MistConnApi
const boost::optional<std::uint16_t>&
ServerResponse::statusCode() const
{
  return _impl->response().statusCode();
}

/*
* ServerResponseImpl
*/

/*
* ServerStream
*/
MistConnApi
ServerStream::ServerStream(std::shared_ptr<ServerStreamImpl> impl)
  : _impl(std::move(impl))
{}

MistConnApi
ServerSession
ServerStream::session()
{
  return ServerSession(_impl->session());
}

MistConnApi
ServerRequest
ServerStream::request()
{
  return ServerRequest(_impl);
}

MistConnApi
ServerResponse
ServerStream::response()
{
  return ServerResponse(_impl);
}

MistConnApi
boost::system::error_code
ServerStream::submitResponse(std::uint16_t statusCode,
  const header_map& headers, generator_callback cb)
{
  return _impl->submitResponse(statusCode, headers, std::move(cb));
}

MistConnApi
void
ServerStream::setOnClose(close_callback cb)
{
  _impl->setOnClose(std::move(cb));
}

MistConnApi
void
ServerStream::close(boost::system::error_code ec)
{
  _impl->close(ec);
}

MistConnApi
void
ServerStream::resume()
{
  _impl->resume();
}

MistConnApi
boost::system::error_code
ServerStream::submitTrailers(const header_map& trailers)
{
  return _impl->submitTrailers(trailers);
}

/*
 * ServerStreamImpl
 */
ServerStreamImpl::ServerStreamImpl(std::shared_ptr<ServerSessionImpl> session)
  : StreamImpl(std::static_pointer_cast<SessionImpl>(session)), _request(*this)
{}

std::shared_ptr<ServerSessionImpl>
ServerStreamImpl::session()
{
  return std::static_pointer_cast<ServerSessionImpl>(StreamImpl::session());
}

ServerRequestImpl&
ServerStreamImpl::request()
{
  return _request;
}

ServerResponseImpl&
ServerStreamImpl::response()
{
  return _response;
}

boost::system::error_code
ServerStreamImpl::submitResponse(std::uint16_t statusCode, header_map headers,
  generator_callback cb)
{
  /* Make the header vector */
  std::vector<nghttp2_nv> nvs;
  {
    /* Special and mandatory headers */
    headers.insert({ ":status",{ std::to_string(statusCode), false } });
    nvs = makeHeaderNv(headers);
    response().setHeaders(headers);
  }

  if (cb) {
    setOnRead(std::move(cb));
  }

  return session()->submitResponse(*this, nvs);
}

boost::system::error_code
ServerStreamImpl::submitTrailers(header_map headers)
{
  return session()->submitTrailers(*this, makeHeaderNv(headers));
}

int
ServerStreamImpl::onHeader(const nghttp2_frame* frame,
  const std::uint8_t* name, std::size_t namelen, const std::uint8_t* value,
  std::size_t valuelen, std::uint8_t flags)
{
  switch (frame->hd.type) {
  case NGHTTP2_HEADERS:
    if (frame->headers.cat != NGHTTP2_HCAT_REQUEST)
      return 0;
  
    return request().onHeader(frame, name, namelen, value, valuelen, flags);
  }
  return 0;
}

int
ServerStreamImpl::onFrameRecv(const nghttp2_frame* frame)
{
  switch (frame->hd.type) {
  case NGHTTP2_DATA: {
    if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
      /* No data */
      request().onData(nullptr, 0);
    break;
  }
  case NGHTTP2_HEADERS: {
    if (frame->headers.cat != NGHTTP2_HCAT_REQUEST)
      return 0;

    if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
      request().onData(nullptr, 0);
    
    break;
  }
  }
  return 0;
}

int
ServerStreamImpl::onFrameSend(const nghttp2_frame* frame)
{
  if (frame->hd.type == NGHTTP2_RST_STREAM) {
    std::cerr << "Server reset stream" << std::endl;
  }
  return 0;
}

int
ServerStreamImpl::onFrameNotSend(const nghttp2_frame* frame, int errorCode)
{
  return 0;
}

int
ServerStreamImpl::onDataChunkRecv(std::uint8_t flags, const std::uint8_t* data,
  std::size_t length)
{
  request().onData(data, length);

  return 0;
}

int
ServerStreamImpl::onStreamClose(std::uint32_t errorCode)
{
  boost::system::error_code ec;
  if (errorCode)
    ec = make_nghttp2_error(errorCode);
  close(ec);

  _request.onData(nullptr, 0);

  return 0;
}

} // namespace h2
} // namespace mist
