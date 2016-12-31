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
  _onClose = std::move(cb);
}

void
StreamImpl::close(boost::system::error_code ec)
{
  /* TODO: rst_stream? */
  if (_onClose)
    _onClose(ec);
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
