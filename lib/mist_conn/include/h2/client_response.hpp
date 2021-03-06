/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>
#include <memory>

#include <boost/optional.hpp>

#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class ClientStream;
class ClientStreamImpl;

class MistConnApi ClientResponse
{
public:

  ClientStream stream();

  void setOnData(data_callback cb);

  const header_map& headers() const;
  const boost::optional<std::uint64_t>& contentLength() const;
  const boost::optional<std::uint16_t>& statusCode() const;

  ClientResponse(std::shared_ptr<ClientStreamImpl> impl);

  ClientResponse(const ClientResponse&) = default;
  ClientResponse& operator=(const ClientResponse&) = default;

  inline bool operator==(const ClientResponse& rhs) const
    { return _impl == rhs._impl; }
  inline bool operator<(const ClientResponse& rhs) const
    { return _impl < rhs._impl; }

  std::shared_ptr<ClientStreamImpl> _impl;

};

}
}
