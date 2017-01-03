/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class ServerStream;
class ServerStreamImpl;

class MistConnApi ServerResponse
{
public:

  ServerStream stream();

  void setOnRead(generator_callback cb);
  void end();
  void end(const std::string& buffer);
  void end(const std::vector<std::uint8_t>& buffer);

  boost::system::error_code submitTrailers(header_map headers);

  const header_map& headers() const;
  const boost::optional<std::uint64_t>& contentLength() const;
  const boost::optional<std::uint16_t>& statusCode() const;

  //boost::optional<ServerRequest> push(boost::system::error_code &ec,
  //  std::string method, std::string path, header_map headers);

  ServerResponse(std::shared_ptr<ServerStreamImpl> impl);

  ServerResponse(const ServerResponse&) = default;
  ServerResponse& operator=(const ServerResponse&) = default;

  inline bool operator==(const ServerResponse& rhs) const
    { return _impl == rhs._impl; }
  inline bool operator<(const ServerResponse& rhs) const
    { return _impl < rhs._impl; }

  std::shared_ptr<ServerStreamImpl> _impl;

};

}
}
