#include <cstddef>
#include <memory>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <nghttp2/nghttp2.h>

#include "h2/session.hpp"
#include "h2/client_response.hpp"
#include "h2/client_session.hpp"
#include "h2/client_session_impl.hpp"
#include "h2/client_stream.hpp"
#include "h2/client_stream_impl.hpp"

namespace mist
{
namespace h2
{

/*
 * ClientRequest
 */
MistConnApi
ClientRequest::ClientRequest(std::shared_ptr<ClientStreamImpl> impl)
  : _impl(impl)
{}

MistConnApi
ClientStream
ClientRequest::stream()
{
  return ClientStream(_impl);
}

MistConnApi
void
ClientRequest::setOnResponse(client_response_callback cb)
{
  _impl->request().setOnResponse(std::move(cb));
}

MistConnApi
void
ClientRequest::setOnPush(client_request_callback cb)
{
  _impl->request().setOnPush(std::move(cb));
}

MistConnApi
void
ClientRequest::setOnRead(generator_callback cb)
{
  _impl->setOnRead(std::move(cb));
}

MistConnApi
void
ClientRequest::end()
{
  _impl->end();
}

MistConnApi
void
ClientRequest::end(const std::string& data)
{
  _impl->end(data);
}

MistConnApi
void
ClientRequest::end(const std::vector<std::uint8_t>& data)
{
  _impl->end(data);
}

MistConnApi
const header_map&
ClientRequest::headers() const
{
  return _impl->request().headers();
}

MistConnApi
const boost::optional<std::uint64_t>&
ClientRequest::contentLength() const
{
  return _impl->request().contentLength();
}

MistConnApi
const boost::optional<std::string>&
ClientRequest::method() const
{
  return _impl->request().method();
}

MistConnApi
const boost::optional<std::string>&
ClientRequest::path() const
{
  return _impl->request().path();
}

MistConnApi
const boost::optional<std::string>&
ClientRequest::scheme() const
{
  return _impl->request().scheme();
}

MistConnApi
const boost::optional<std::string>&
ClientRequest::authority() const
{
  return _impl->request().authority();
}

/*
 * ClientRequestImpl
 */
ClientRequestImpl::ClientRequestImpl(ClientStreamImpl& stream)
  : _stream(stream)
{}

void
ClientRequestImpl::setOnResponse(client_response_callback cb)
{
  _onResponse = std::move(cb);
}

void
ClientRequestImpl::setOnPush(client_request_callback cb)
{
  _onPush = std::move(cb);
}

void
ClientRequestImpl::onResponse()
{
  if (_onResponse) {
    _onResponse(ClientResponse(
      std::static_pointer_cast<ClientStreamImpl>(_stream.shared_from_this())));
  }
}

void
ClientRequestImpl::onPush(std::shared_ptr<ClientStreamImpl> pushedStream)
{
  if (_onPush)
    _onPush(ClientRequest(std::move(pushedStream)));
}

boost::system::error_code
ClientRequestImpl::submitTrailers(header_map headers)
{
  return _stream.submitTrailers(std::move(headers));
}

/*
 * ClientResponse
 */
MistConnApi
ClientResponse::ClientResponse(std::shared_ptr<ClientStreamImpl> impl)
  : _impl(impl)
{}

MistConnApi
ClientStream
ClientResponse::stream()
{
  return ClientStream(_impl);
}

MistConnApi
void
ClientResponse::setOnData(data_callback cb)
{
  _impl->response().setOnData(std::move(cb));
}

MistConnApi
const header_map&
ClientResponse::headers() const
{
  return _impl->response().headers();
}

MistConnApi
const boost::optional<std::uint64_t>&
ClientResponse::contentLength() const
{
  return _impl->response().contentLength();
}

MistConnApi
const boost::optional<std::uint16_t>&
ClientResponse::statusCode() const
{
  return _impl->response().statusCode();
}

/*
 * ClientResponseImpl
 */
ClientResponseImpl::ClientResponseImpl(ClientStreamImpl& stream)
  : _stream(stream)
{}

void
ClientResponseImpl::setOnData(data_callback cb)
{
  _onData = std::move(cb);
}

void
ClientResponseImpl::onData(const std::uint8_t* data, std::size_t length)
{
  if (_onData)
    _onData(data, length);
}

/*
* ClientStream
*/
MistConnApi
ClientStream::ClientStream(std::shared_ptr<ClientStreamImpl> impl)
  : _impl(impl)
{}

MistConnApi
ClientSession
ClientStream::session()
{
  return ClientSession(_impl->session());
}

MistConnApi
ClientRequest
ClientStream::request()
{
  return ClientRequest(_impl);
}

MistConnApi
ClientResponse
ClientStream::response()
{
  return ClientResponse(_impl);
}

MistConnApi
void
ClientStream::setOnClose(close_callback cb)
{
  _impl->setOnClose(std::move(cb));
}

MistConnApi
void
ClientStream::close(boost::system::error_code ec)
{
  _impl->close(std::move(ec));
}

MistConnApi
void
ClientStream::resume()
{
  _impl->resume();
}

MistConnApi
boost::system::error_code
ClientStream::submitTrailers(const header_map& trailers)
{
  return _impl->submitTrailers(trailers);
}

/*
 * ClientStreamImpl
 */
ClientStreamImpl::ClientStreamImpl(std::shared_ptr<ClientSessionImpl> session)
  : StreamImpl(std::static_pointer_cast<SessionImpl>(session)),
    _request(*this),
    _response(*this)
    {}

std::shared_ptr<ClientSessionImpl>
ClientStreamImpl::session()
{
  return std::static_pointer_cast<ClientSessionImpl>(StreamImpl::session());
}

ClientRequestImpl&
ClientStreamImpl::request()
{
  return _request;
}

ClientResponseImpl&
ClientStreamImpl::response()
{
  return _response;
}

boost::system::error_code
ClientStreamImpl::submit(std::string method, std::string path,
  std::string scheme, std::string authority, header_map headers,
  generator_callback cb)
{
  /* Make the header vector */
  std::vector<nghttp2_nv> nvs;
  {
    /* Special and mandatory headers */
    headers.insert({ ":method",{ std::move(method), false } });
    headers.insert({ ":path",{ std::move(path), false } });
    headers.insert({ ":scheme",{ std::move(scheme), false } });
    headers.insert({ ":authority",{ std::move(authority), false } });
    nvs = makeHeaderNv(headers);
    request().setHeaders(headers);
  }

  if (cb) {
    setOnRead(std::move(cb));
  }

  /* Submit to the session */
  return session()->submitRequest(*this, nvs);
}

boost::system::error_code
ClientStreamImpl::submitTrailers(header_map headers)
{
  return session()->submitTrailers(*this, makeHeaderNv(headers));
}

//void
//ClientStreamImpl::onPush(std::shared_ptr<ClientStreamImpl> pushedStream)
//{
//  request().onPush(std::move(pushedStream));
//}

void ClientStreamImpl::onPush(std::shared_ptr<ClientStreamImpl> strm)
{
  request().onPush(std::move(strm));
}

int
ClientStreamImpl::onHeader(const nghttp2_frame* frame,
  const std::uint8_t* name, std::size_t namelen, const std::uint8_t* value,
  std::size_t valuelen, std::uint8_t flags)
{
  switch (frame->hd.type) {
  case NGHTTP2_HEADERS:
    return response().onHeader(frame, name, namelen, value, valuelen, flags);
  case NGHTTP2_PUSH_PROMISE:
    return request().onHeader(frame, name, namelen, value, valuelen, flags);
  }
  return 0;
}

int
ClientStreamImpl::onFrameRecv(const nghttp2_frame* frame)
{
  switch (frame->hd.type) {
  case NGHTTP2_DATA: {
    if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
      /* No data */
      response().onData(nullptr, 0);
    
    break;
  }
  case NGHTTP2_HEADERS: {
    bool expectsFinalResponse = response().statusCode()
      && *response().statusCode() >= 100 && *response().statusCode() < 200;
    
    if (frame->headers.cat == NGHTTP2_HCAT_HEADERS &&
        !expectsFinalResponse) {
      // Ignore trailers
      return 0;
    }

    if (expectsFinalResponse) {
      // Wait for final response
      return 0;
    }
    
    request().onResponse();
    
    if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
      response().onData(nullptr, 0);
    }
    
    break;
  }
  }
  return 0;
}

int
ClientStreamImpl::onFrameSend(const nghttp2_frame* frame)
{
  return 0;
}

int
ClientStreamImpl::onFrameNotSend(const nghttp2_frame* frame, int errorCode)
{
  return 0;
}

int
ClientStreamImpl::onDataChunkRecv(std::uint8_t flags, const std::uint8_t* data,
  std::size_t length)
{
  response().onData(data, length);

  return 0;
}

int
ClientStreamImpl::onStreamClose(std::uint32_t errorCode)
{
  close(make_nghttp2_error(static_cast<nghttp2_error>(errorCode)));

  if (_response._onData)
    _response._onData(nullptr, 0);

  return 0;
}

} // namespace h2
} // namespace mist
