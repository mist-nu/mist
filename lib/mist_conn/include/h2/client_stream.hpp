#ifndef __MIST_INCLUDE_H2_CLIENT_STREAM_HPP__
#define __MIST_INCLUDE_H2_CLIENT_STREAM_HPP__

#include <memory>

#include <boost/system/error_code.hpp>

#include "h2/types.hpp"
#include "h2/stream.hpp"

namespace mist
{
namespace h2
{

class ClientSession;
class ClientRequest;
class ClientResponse;
class ClientStreamImpl;

class ClientStream : public Stream
{
public:

  ClientSession session();

  ClientRequest request();

  ClientResponse response();

  virtual void setOnClose(close_callback cb) override;

  virtual void close(boost::system::error_code ec) override;

  virtual void resume() override;

  virtual boost::system::error_code submitTrailers(
    const header_map& trailers) override;

  ClientStream(std::shared_ptr<ClientStreamImpl> impl);

  ClientStream(const ClientStream&) = default;
  ClientStream& operator=(const ClientStream&) = default;

  inline bool operator==(const ClientStream& rhs) const
    { return _impl == rhs._impl; }
  inline bool operator<(const ClientStream& rhs) const
    { return _impl < rhs._impl; }

  std::shared_ptr<ClientStreamImpl> _impl;

};

} // namespace h2
} // namespace mist

#endif
