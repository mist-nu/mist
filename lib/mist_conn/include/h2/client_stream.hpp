/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

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

class MistConnApi ClientStream : public Stream
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
