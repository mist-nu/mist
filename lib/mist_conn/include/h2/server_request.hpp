#ifndef __MIST_INCLUDE_H2_SERVER_REQUEST_HPP__
#define __MIST_INCLUDE_H2_SERVER_REQUEST_HPP__

#include <cstddef>
#include <memory>
#include <string>

#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class ServerStream;
class ServerStreamImpl;

class ServerRequest
{
public:

  ServerStream stream();

  const header_map& headers() const;
  const boost::optional<std::uint64_t>& contentLength() const;
  const boost::optional<std::string>& method() const;
  const boost::optional<std::string>& path() const;
  const boost::optional<std::string>& scheme() const;
  const boost::optional<std::string>& authority() const;

  void setOnData(data_callback cb);

  ServerRequest(std::shared_ptr<ServerStreamImpl>);

  ServerRequest(const ServerRequest&) = default;
  ServerRequest& operator=(const ServerRequest&) = default;

  inline bool operator==(const ServerRequest& rhs) const
    { return _impl == rhs._impl; }
  inline bool operator<(const ServerRequest& rhs) const
    { return _impl < rhs._impl; }

  std::shared_ptr<ServerStreamImpl> _impl;

};

}
}

#endif
