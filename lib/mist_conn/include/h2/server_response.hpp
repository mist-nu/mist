/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_INCLUDE_H2_SERVER_RESPONSE_HPP__
#define __MIST_INCLUDE_H2_SERVER_RESPONSE_HPP__

#include <cstddef>
#include <memory>

#include <boost/optional.hpp>

#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class ServerStream;
class ServerStreamImpl;

class ServerResponse
{
public:

  ServerStream stream();

  void setOnRead(generator_callback cb);

  void end();

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

#endif
