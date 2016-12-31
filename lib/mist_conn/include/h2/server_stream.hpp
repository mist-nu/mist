/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_INCLUDE_H2_SERVER_STREAM_HPP__
#define __MIST_INCLUDE_H2_SERVER_STREAM_HPP__

#include <cstddef>
#include <memory>

#include <boost/system/error_code.hpp>

#include "h2/stream.hpp"
#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class ServerSession;
class ServerRequest;
class ServerResponse;
class ServerStreamImpl;

class ServerStream : public Stream
{
public:

  ServerSession session();

  ServerRequest request();

  ServerResponse response();

  boost::system::error_code submitResponse(std::uint16_t statusCode,
    const header_map& headers, generator_callback cb = nullptr);

  virtual void setOnClose(close_callback cb) override;

  virtual void close(boost::system::error_code ec) override;

  virtual void resume() override;

  virtual boost::system::error_code submitTrailers(
    const header_map& trailers) override;

  ServerStream(std::shared_ptr<ServerStreamImpl> impl);

  ServerStream(const ServerStream&) = default;
  ServerStream& operator=(const ServerStream&) = default;

  inline bool operator==(const ServerStream& rhs) const
    { return _impl == rhs._impl; }
  inline bool operator<(const ServerStream& rhs) const
    { return _impl < rhs._impl; }

  std::shared_ptr<ServerStreamImpl> _impl;

};

} // namespace h2
} // namespace mist

#endif
