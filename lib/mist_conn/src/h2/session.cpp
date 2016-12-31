/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
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

#include "h2/session.hpp"
#include "h2/session_impl.hpp"
#include "h2/stream_impl.hpp"

namespace mist
{
namespace h2
{

namespace
{
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::string frameName(std::uint8_t type)
{
  switch (type) {
  case NGHTTP2_DATA: return "NGHTTP2_DATA";
  case NGHTTP2_HEADERS: return "NGHTTP2_HEADERS";
  case NGHTTP2_PRIORITY: return "NGHTTP2_PRIORITY";
  case NGHTTP2_RST_STREAM: return "NGHTTP2_RST_STREAM";
  case NGHTTP2_SETTINGS: return "NGHTTP2_SETTINGS";
  case NGHTTP2_PUSH_PROMISE: return "NGHTTP2_PUSH_PROMISE";
  case NGHTTP2_PING: return "NGHTTP2_PING";
  case NGHTTP2_GOAWAY: return "NGHTTP2_GOAWAY";
  case NGHTTP2_WINDOW_UPDATE: return "NGHTTP2_WINDOW_UPDATE";
  case NGHTTP2_CONTINUATION: return "NGHTTP2_CONTINUATION";
  //case NGHTTP2_ALTSVC: return "NGHTTP2_ALTSVC";
  default: return "Unknown frame type";
  }
}
} // namespace


/*
 * Session
 */
Session::~Session() {}

/*
 * SessionImpl
 */
SessionImpl::SessionImpl(std::shared_ptr<io::Socket> socket, bool isServer)
  : _socket(std::move(socket)),
    _h2session(to_unique<nghttp2_session>()),
    _sending(false),
    _stopped(false),
    _insideCallback(false)
{
  /* nghttp2 session options */
  c_unique_ptr<nghttp2_option> opts;
  {
    nghttp2_option* optsPtr = nullptr;
    nghttp2_option_new(&optsPtr);
    opts = to_unique(optsPtr);

    nghttp2_option_set_no_http_messaging(opts.get(), 1);
  }

  /* Create the nghttp2_session_callbacks object */
  c_unique_ptr<nghttp2_session_callbacks> cbs;
  {
    nghttp2_session_callbacks* cbsPtr = nullptr;
    nghttp2_session_callbacks_new(&cbsPtr);
    cbs = to_unique(cbsPtr);
  }
  
  /* Set on error callback; for debugging purposes */
  //nghttp2_session_callbacks_set_error_callback(cbs.get(),
  //  [](nghttp2_session* /*session*/, const char* message, std::size_t length,
  //     void* user_data) -> int
  //{
  //  std::cerr << "nghttp2 signalled error: "
  //    << std::string(message, length) << std::endl;
  //  return 0;
  //});
  
  /* Set on begin headers callback */
  nghttp2_session_callbacks_set_on_begin_headers_callback(cbs.get(),
    [](nghttp2_session* /*session*/, const nghttp2_frame* frame,
       void* user_data) -> int
  {
    return static_cast<SessionImpl*>(user_data)->onBeginHeaders(frame);
  });

  /* Set on header callback */
  nghttp2_session_callbacks_set_on_header_callback(cbs.get(),
    [](nghttp2_session* /*session*/, const nghttp2_frame* frame,
       const std::uint8_t* name, std::size_t namelen,
       const std::uint8_t* value, std::size_t valuelen, std::uint8_t flags,
       void* user_data) -> int
  {
    return static_cast<SessionImpl*>(user_data)->onHeader(frame, name, namelen,
                                                       value, valuelen, flags);
  });
  
  /* Set on stream frame send callback */
  nghttp2_session_callbacks_set_on_frame_send_callback(cbs.get(),
    [](nghttp2_session* /*session*/, const nghttp2_frame* frame,
       void* user_data) -> int
  {
    return static_cast<SessionImpl*>(user_data)->onFrameSend(frame);
  });
  
  /* Set on frame not send callback */
  nghttp2_session_callbacks_set_on_frame_not_send_callback(cbs.get(),
    [](nghttp2_session* /*session*/, const nghttp2_frame *frame,
       int error_code, void* user_data) -> int
  {
    return static_cast<SessionImpl*>(user_data)->onFrameNotSend(
      frame, error_code);
  });
  
  /* Set on frame receive callback */
  nghttp2_session_callbacks_set_on_frame_recv_callback(cbs.get(),
    [](nghttp2_session* /*session*/, const nghttp2_frame *frame,
       void* user_data) -> int
  {
    return static_cast<SessionImpl*>(user_data)->onFrameRecv(frame);
  });
  
  /* Set on data chunk receive callback */
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(cbs.get(),
    [](nghttp2_session* /*session*/, std::uint8_t flags,
       std::int32_t stream_id, const std::uint8_t* data, std::size_t len,
       void* user_data) -> int
  {
    return static_cast<SessionImpl*>(user_data)->onDataChunkRecv(
      flags, stream_id, data, len);
  });

  /* Set on stream close callback */
  nghttp2_session_callbacks_set_on_stream_close_callback(cbs.get(),
    [](nghttp2_session* /*session*/, std::int32_t stream_id,
       std::uint32_t error_code, void *user_data) -> int
  {
    return static_cast<SessionImpl*>(user_data)->onStreamClose(stream_id,
      error_code);
  });
  
  /* Create the nghttp2_session and assign it to h2session */
  {
    nghttp2_session* sessPtr = nullptr;
    int rv;
    if (isServer) {
      rv = nghttp2_session_server_new2(&sessPtr, cbs.get(), this, opts.get());
    } else {
      rv = nghttp2_session_client_new2(&sessPtr, cbs.get(), this, opts.get());
    }
    if (rv) {
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_nghttp2_error(static_cast<nghttp2_error>(rv)),
        "Unable to create nghttp2 session"));
    }
    _h2session = to_unique(sessPtr);
  }
  
  /* Send connection settings */
  {
    std::vector<nghttp2_settings_entry> iv{
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100},
    };
    auto rv = nghttp2_submit_settings(_h2session.get(), NGHTTP2_FLAG_NONE,
                                      iv.data(), iv.size());
    if (rv) {
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_nghttp2_error(static_cast<nghttp2_error>(rv)),
        "Unable to send HTTP/2 settings"));
    }
  }
}

SessionImpl::~SessionImpl() {}

boost::optional<std::shared_ptr<StreamImpl>>
SessionImpl::findBaseStream(std::int32_t streamId)
{
  std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
  auto it = _streams.find(streamId);
  if (it == _streams.end()) {
    /* We know of no such id */
    return boost::none;
  }
  auto ptr = it->second.lock();
  if (!ptr) {
    /* The weak pointer has expired; reset the stream */
    nghttp2_submit_rst_stream(nghttp2Session(), NGHTTP2_FLAG_NONE, streamId,
      NGHTTP2_INTERNAL_ERROR);
    _streams.erase(streamId);
    return boost::none;
  }
  return ptr;
}

void
SessionImpl::insertBaseStream(std::shared_ptr<StreamImpl> strm)
{
  std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
  assert(strm);
  assert(strm->hasValidStreamId());
  _streams.insert(std::make_pair(strm->streamId(),
    std::weak_ptr<StreamImpl>(strm)));
}

namespace
{
struct FrameGuard {
  FrameGuard(bool& flag) : _flag(flag) { assert (!_flag); _flag = true; }
  ~FrameGuard() { assert (_flag); _flag = false; }
  bool& _flag;
};
}

/*
 * Read.
 */
void
SessionImpl::readCallback(const std::uint8_t* data, std::size_t length,
  boost::system::error_code ec)
{
  if (ec) {
    /* Read error */
    error(ec);
    return;
  }

  if (length == 0) {
    /* Socket closed */
    stop();
    return;
  }
  
  {
    std::unique_lock<std::recursive_mutex> lock(_sessionMutex);
    auto nrecvd = nghttp2_session_mem_recv(_h2session.get(), data, length);
    if (nrecvd < 0) {
      error(make_nghttp2_error(static_cast<nghttp2_error>(nrecvd)));
    } else if (nrecvd != length) {
      error(make_nghttp2_error(NGHTTP2_ERR_PROTO));
    }
  }
  
  /* nghttp2 might have decided that it has things to send */
  write();
}

/* Start reading from the socket and write the first piece of data */
void
SessionImpl::start()
{
  /* Bind the socket read callback */
  {
    using namespace std::placeholders;
    auto anchor(shared_from_this());
    _socket->read([this, anchor](const uint8_t* data, std::size_t length,
        boost::system::error_code ec)
    {
      readCallback(data, length, ec);
    });
    //_socket->read(//std::bind(&SessionImpl::readCallback, this, _1, _2, _3));
  }
  
  /* Write the first data */
  write();
}

nghttp2_session*
SessionImpl::nghttp2Session()
{
  return _h2session.get();
}

bool
SessionImpl::isStopped() const
{
  return _stopped;
}

bool
SessionImpl::alive() const
{
  std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
  return nghttp2_session_want_read(_h2session.get())
      || nghttp2_session_want_write(_h2session.get())
      || _sending;
}

void
SessionImpl::setOnError(error_callback cb)
{
  _onError = std::move(cb);
}

void
SessionImpl::error(boost::system::error_code ec)
{
  if (_onError)
    _onError(ec);
  stop();
}

void
SessionImpl::stop()
{
  if (_stopped)
    return;
  _stopped = true;
  for (auto s : _streams) {
    auto stream(s.second.lock());
    if (stream) {
        stream->close(boost::system::error_code());
    }
    nghttp2_submit_rst_stream(nghttp2Session(), 0, s.first, NGHTTP2_NO_ERROR);
  }
  _socket->close();
}

void
SessionImpl::shutdown()
{
  if (_stopped)
    return;

  {
    std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
    nghttp2_session_terminate_session(_h2session.get(), NGHTTP2_NO_ERROR);
  }

  write();
}

/* TODO: Is there a way to get more of a request-based write? I.e. the IOContext demands write data... */
void
SessionImpl::write()
{
  if (_insideCallback)
    return;
  
  if (_sending)
    /* There is already a send in progress */
    return;
    
  _sending = true;
  
  const uint8_t* data;
  std::size_t length;
  {
    FrameGuard guard(_insideCallback);
    
    ssize_t nsend;
    {
      std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
      nsend = nghttp2_session_mem_send(_h2session.get(), &data);
    }

    if (nsend < 0) {
      /* Error */
      error(make_nghttp2_error(static_cast<nghttp2_error>(nsend)));
      return;
    } else if (nsend == 0) {
      /* No more data to send */
      _sending = false;
      return;
    } else {
      length = nsend;
    }
  }
  
  _socket->write(data, length,
    [=] // , anchor(shared_from_this())
    (std::size_t nsent, boost::system::error_code ec)
  {
    _sending = false;
    if (ec) {
      error(make_nghttp2_error(static_cast<nghttp2_error>(length)));
    } else if (length != nsent) {
      error(make_mist_error(MIST_ERR_ASSERTION));
    }
    write();
  });
}

boost::system::error_code
SessionImpl::submitRequest(StreamImpl& strm,
  const std::vector<nghttp2_nv>& nvs)
{
  assert(!strm.hasValidStreamId());

  /* Set the data provider */
  nghttp2_data_provider* prdptr = nullptr;
  nghttp2_data_provider prd;
  {
    prd.source.ptr = this;
    prd.read_callback =
      [](nghttp2_session* /*session*/, std::int32_t stream_id,
        std::uint8_t* data, std::size_t length, std::uint32_t* flags,
        nghttp2_data_source* source, void* userp) -> ssize_t
    {
      StreamImpl& strm = *static_cast<StreamImpl*>(source->ptr);
      return static_cast<ssize_t>(strm.onRead(data, length, flags));
    };
    prdptr = &prd;
  }

  /* Submit */
  std::int32_t streamId;
  {
    std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
    streamId = nghttp2_submit_request(nghttp2Session(),
      nullptr, nvs.data(), nvs.size(), prdptr, &strm);
  }

  if (streamId < 0) {
    return make_nghttp2_error(streamId);
  }

#ifdef _DEBUG
  /* TODO: For testing, randomly throw NGHTTP2_ERR_STREAM_ID_NOT_AVAILABLE */
#endif

  strm.setStreamId(streamId);

  return boost::system::error_code();
}

boost::system::error_code
SessionImpl::submitResponse(StreamImpl& strm,
  const std::vector<nghttp2_nv>& nvs)
{
  /* Set the data provider */
  nghttp2_data_provider* prdptr = nullptr;
  nghttp2_data_provider prd;
  {
    prd.source.ptr = this;
    prd.read_callback =
      [](nghttp2_session* /*session*/, std::int32_t stream_id,
        std::uint8_t* data, std::size_t length, std::uint32_t* flags,
        nghttp2_data_source* source, void* userp) -> ssize_t
    {
      StreamImpl& strm = *static_cast<StreamImpl*>(source->ptr);
      return static_cast<ssize_t>(strm.onRead(data, length, flags));
    };
    prdptr = &prd;
  }

  /* Submit */
  int rv;
  {
    std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
    rv = nghttp2_submit_response(nghttp2Session(), strm.streamId(),
      nvs.data(), nvs.size(), prdptr);
  }

  if (rv) {
    return make_nghttp2_error(rv);
  }

  return boost::system::error_code();
}

boost::system::error_code
SessionImpl::submitTrailers(StreamImpl& strm,
  const std::vector<nghttp2_nv>& nvs)
{
  assert(strm.hasValidStreamId());

  int rv;
  {
    std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
    rv = nghttp2_submit_trailer(nghttp2Session(), strm.streamId(),
      nvs.data(), nvs.size());
  }

  if (rv) {
    return make_nghttp2_error(rv);
  }

  return boost::system::error_code();
}

boost::system::error_code
SessionImpl::resumeData(StreamImpl& strm)
{
  std::lock_guard<std::recursive_mutex> lock(_sessionMutex);
  int rv = nghttp2_session_resume_data(nghttp2Session(), strm.streamId());

  if (rv) {
    return make_nghttp2_error(rv);
  }

  return boost::system::error_code();
}

} // namespace h2
} // namespace mist
