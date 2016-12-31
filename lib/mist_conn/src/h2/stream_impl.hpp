/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_SRC_H2_STREAM_STREAM_IMPL_HPP__
#define __MIST_SRC_H2_STREAM_STREAM_IMPL_HPP__

#include <cstddef>
#include <memory>
#include <string>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <nghttp2/nghttp2.h>

#include "error/nghttp2.hpp"
#include "error/mist.hpp"

#include "memory/nghttp2.hpp"

#include "h2/types.hpp"
#include "h2/util.hpp"
#include "h2/stream.hpp"
#include "h2/client_request.hpp"
#include "h2/client_response.hpp"
#include "h2/server_request.hpp"
#include "h2/server_response.hpp"

namespace mist
{
namespace h2
{

class SessionImpl;

class StreamImpl : public std::enable_shared_from_this<StreamImpl>
{
public:

  virtual ~StreamImpl();

  std::shared_ptr<SessionImpl> session();

  void setStreamId(std::int32_t streamId);

  bool hasValidStreamId() const;

  std::int32_t streamId() const;

  void setOnClose(close_callback cb);

  void close(boost::system::error_code ec);

  void resume();

private:

  friend class SessionImpl;

  std::shared_ptr<SessionImpl> _session;

  /* RFC7540 5.1.1
  * "Streams are identified with an unsigned 31-bit integer."
  * We make use of int32_t, and use negative integers to indicate
  * that the stream id is invalid.
  */
  std::int32_t _streamId;

  close_callback _onClose;

protected:

  StreamImpl(std::shared_ptr<SessionImpl> session);
  
  virtual int onHeader(const nghttp2_frame* frame, const std::uint8_t* name,
    std::size_t namelen, const std::uint8_t* value, std::size_t valuelen,
    std::uint8_t flags) = 0;

  virtual int onFrameSend(const nghttp2_frame* frame) = 0;

  virtual int onFrameNotSend(const nghttp2_frame* frame, int errorCode) = 0;

  virtual int onFrameRecv(const nghttp2_frame* frame) = 0;

  virtual int onDataChunkRecv(std::uint8_t flags, const std::uint8_t* data,
    std::size_t len) = 0;

  virtual int onStreamClose(std::uint32_t errorCode) = 0;

  virtual generator_callback::result_type onRead(std::uint8_t* data,
    std::size_t length, std::uint32_t* flags) = 0;

};

} // namespace h2
} // namespace mist

#endif
