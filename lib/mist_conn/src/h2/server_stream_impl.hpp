/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#ifndef __MIST_SRC_H2_SERVER_STREAM_IMPL_HPP__
#define __MIST_SRC_H2_SERVER_STREAM_IMPL_HPP__

#include <cstddef>
#include <memory>
#include <string>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <nghttp2/nghttp2.h>

#include "error/nghttp2.hpp"
#include "error/mist.hpp"

#include "memory/nghttp2.hpp"

#include "h2/lane.hpp"
#include "h2/types.hpp"
#include "h2/util.hpp"

#include "h2/server_request.hpp"
#include "h2/server_response.hpp"
#include "h2/server_stream.hpp"
#include "h2/server_session_impl.hpp"
#include "h2/stream_impl.hpp"

namespace mist
{
namespace h2
{

class ServerStreamImpl;

/*
 * ServerRequestImpl
 */

class ServerRequestImpl : public RequestLane
{
public:

  ServerRequestImpl(ServerStreamImpl& stream);

  void setOnData(data_callback cb);

protected:

  friend class ServerStreamImpl;

  void onData(const std::uint8_t* data, std::size_t length);

private:

  ServerStreamImpl& _stream;

  data_callback _onData;

};

/*
* ServerResponseImpl
*/

class ServerResponseImpl : public ResponseLane
{
private:

  friend class ServerStreamImpl;

protected:

  //boost::optional<ServerRequest> push(boost::system::error_code &ec,
  //  std::string method, std::string path, header_map headers);

};

/*
* ServerStreamImpl
*/

class ServerStreamImpl : public StreamImpl
{
public:

  ServerStreamImpl(std::shared_ptr<ServerSessionImpl> session);

  std::shared_ptr<ServerSessionImpl> session();

  ServerRequestImpl& request();

  ServerResponseImpl& response();

  boost::system::error_code submitResponse(std::uint16_t statusCode,
    header_map headers, generator_callback cb);

  boost::system::error_code submitTrailers(header_map headers);

private:

  friend class ServerSessionImpl;

  ServerRequestImpl _request;

  ServerResponseImpl _response;

protected:

  virtual int onHeader(const nghttp2_frame* frame, const std::uint8_t* name,
    std::size_t namelen, const std::uint8_t* value, std::size_t valuelen,
    std::uint8_t flags) override;

  virtual int onFrameSend(const nghttp2_frame* frame) override;

  virtual int onFrameNotSend(const nghttp2_frame* frame, int errorCode)
    override;

  virtual int onFrameRecv(const nghttp2_frame* frame) override;

  virtual int onDataChunkRecv(std::uint8_t flags, const std::uint8_t* data,
    std::size_t len) override;

  virtual int onStreamClose(std::uint32_t errorCode) override;

};

} // namespace h2
} // namespace mist

#endif
