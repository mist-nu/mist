/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <cstddef>
#include <memory>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <nghttp2/nghttp2.h>

#include "h2/session.hpp"
#include "h2/session_impl.hpp"
#include "h2/stream.hpp"
#include "h2/stream_impl.hpp"

namespace mist
{
namespace h2
{

/*
 * Stream
 */
MistConnApi
Stream::~Stream() {}

/*
 * StreamImpl
 */
StreamImpl::StreamImpl(std::shared_ptr<SessionImpl> session)
  : _session(session),
    _streamId(-1)
{}

StreamImpl::~StreamImpl() {}

std::shared_ptr<SessionImpl>
StreamImpl::session()
{
  return _session;
}

bool
StreamImpl::hasValidStreamId() const
{
  /* TODO: 0x0 is the control stream, do we touch it directly? */
  return _streamId >= 0;
}

std::int32_t
StreamImpl::streamId() const
{
  return _streamId;
}

void
StreamImpl::setStreamId(std::int32_t streamId)
{
  assert (!hasValidStreamId() && "May set stream id only once");
  _streamId = streamId;
}

void
StreamImpl::setOnClose(close_callback cb)
{
  std::unique_lock<std::recursive_mutex> lock(mux);

  _onClose = std::move(cb);
}

void
StreamImpl::close(boost::system::error_code ec)
{
  std::unique_lock<std::recursive_mutex> lock(mux);

  /* TODO: rst_stream? */
  if (_onClose)
    _onClose(ec);
}

void
StreamImpl::setOnRead(generator_callback cb)
{
  std::unique_lock<std::recursive_mutex> lock(mux);

  auto anchor(shared_from_this());
  /* Extend the lifetime of this object as long as _onRead exists */
  _onRead = [anchor, cb](std::uint8_t* data, std::size_t length)
    -> generator_callback::result_type
  {
    return cb(data, length);
  };
  resume();
}

void
StreamImpl::end()
{
  setOnRead(
    [](std::uint8_t* data, std::size_t length)
    -> boost::optional<std::size_t>
  {
    return 0;
  });
}

void
StreamImpl::end(const std::string& buffer)
{
  std::size_t i = 0;
  setOnRead(
    [buffer, i](std::uint8_t* data, std::size_t length) mutable
    -> boost::optional<std::size_t>
  {
    std::size_t n = std::min(length, buffer.length() - i);
    if (n > 0) {
      const std::uint8_t* begin
        = reinterpret_cast<const std::uint8_t*>(buffer.data() + i);
      const std::uint8_t* end = begin + n;
      std::copy(begin, end, data);
      i += n;
      return n;
    } else {
      return 0;
    }
  });
}

void
StreamImpl::end(const std::vector<uint8_t>& buffer)
{
  std::size_t i = 0;
  setOnRead(
    [buffer, i](std::uint8_t* data, std::size_t length) mutable
    -> boost::optional<std::size_t>
  {
    std::size_t n = std::min(length, buffer.size() - i);
    if (n > 0) {
      const std::uint8_t* begin
        = reinterpret_cast<const std::uint8_t*>(buffer.data() + i);
      const std::uint8_t* end = begin + n;
      std::copy(begin, end, data);
      i += n;
      return n;
    } else {
      return 0;
    }
  });
}

ssize_t
StreamImpl::onRead(std::uint8_t* data, std::size_t length,
  std::uint32_t* flags)
{
  if (_onRead) {
    auto rv(_onRead(data, length));
    if (!rv)
      return NGHTTP2_ERR_DEFERRED;
    if (*rv == 0) {
      *flags |= NGHTTP2_DATA_FLAG_EOF;
      /* No more data to read, erase the callback */
      _onRead = nullptr;
    }
    return static_cast<ssize_t>(*rv);
  } else {
    return NGHTTP2_ERR_DEFERRED;
  }
}

// void
// Stream::cancel(std::uint32_t errorCode)
// {
  // if (session().isStopped()) {
    // /* The whole session is stopped */
    // return;
  // }

  // nghttp2_submit_rst_stream(session().nghttp2Session(), NGHTTP2_FLAG_NONE, streamId(), errorCode);

  // session().signalWrite();
// }

void
StreamImpl::resume()
{
  if (_session->isStopped()) {
    return;
  }

  session()->resumeData(*this);
}

} // namespace h2
} // namespace mist
