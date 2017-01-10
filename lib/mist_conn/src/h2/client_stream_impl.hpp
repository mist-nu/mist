/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#ifndef __MIST_SRC_H2_STREAM_CLIENT_STREAM_IMPL_HPP__
#define __MIST_SRC_H2_STREAM_CLIENT_STREAM_IMPL_HPP__

#include <cstddef>
#include <string>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <nghttp2/nghttp2.h>

#include "error/nghttp2.hpp"
#include "error/mist.hpp"

#include "memory/nghttp2.hpp"

#include "h2/client_request.hpp"
#include "h2/client_response.hpp"
#include "h2/lane.hpp"
#include "h2/server_request.hpp"
#include "h2/server_response.hpp"
#include "h2/stream_impl.hpp"
#include "h2/types.hpp"
#include "h2/util.hpp"

namespace mist
{
namespace h2
{

/*
* ClientRequestImpl
*/
class ClientRequestImpl : public RequestLane
{
public:

  ClientRequestImpl(ClientStreamImpl& stream);

  void setOnResponse(client_response_callback cb);

  void setOnPush(client_request_callback cb);

  boost::system::error_code submitTrailers(header_map headers);

protected:

  friend class ClientStreamImpl;

  void onResponse();

  void onPush(std::shared_ptr<ClientStreamImpl> pushedStream);

private:

  ClientStreamImpl& _stream;

  client_response_callback _onResponse;

  client_request_callback _onPush;

};

/*
* ClientResponseImpl
*/
class ClientResponseImpl : public ResponseLane
{
public:

  ClientResponseImpl(ClientStreamImpl& stream);

  void setOnData(data_callback cb);

protected:

  friend class ClientStreamImpl;

  void onData(const std::uint8_t *data, std::size_t length);

private:

  ClientStreamImpl& _stream;

  data_callback _onData;

};

/*
 * ClientStreamImpl
 */
class ClientStreamImpl : public StreamImpl
{
public:

  ClientStreamImpl(std::shared_ptr<ClientSessionImpl> session);

  std::shared_ptr<ClientSessionImpl> session();

  ClientRequestImpl& request();

  ClientResponseImpl& response();

  boost::system::error_code submit(std::string method, std::string path,
    std::string scheme, std::string authority, header_map headers,
    generator_callback cb);

  boost::system::error_code submitTrailers(header_map headers);

private:

  friend class ClientSessionImpl;

  ClientRequestImpl _request;

  ClientResponseImpl _response;

protected:

  void onPush(std::shared_ptr<ClientStreamImpl> strm);

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
