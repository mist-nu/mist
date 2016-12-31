#ifndef __MIST_HEADERS_H2_CLIENT_REQUEST_HPP__
#define __MIST_HEADERS_H2_CLIENT_REQUEST_HPP__

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

class ClientRequest
{
public:

  ClientStream stream();

  void setOnResponse(client_response_callback cb);

  void setOnPush(client_request_callback cb);

  void setOnRead(generator_callback cb);

  void end();

  const header_map& headers() const;
  const boost::optional<std::uint64_t>& contentLength() const;
  const boost::optional<std::string>& method() const;
  const boost::optional<std::string>& path() const;
  const boost::optional<std::string>& scheme() const;
  const boost::optional<std::string>& authority() const;

  ClientRequest(std::shared_ptr<ClientStreamImpl> impl);

  ClientRequest(const ClientRequest&) = default;
  ClientRequest& operator=(const ClientRequest&) = default;

  inline bool operator==(const ClientRequest& rhs) const
    { return _impl == rhs._impl; }
  inline bool operator<(const ClientRequest& rhs) const
    { return _impl < rhs._impl; }

  std::shared_ptr<ClientStreamImpl> _impl;

};

} // namespace h2
} // namespace mist

#endif
