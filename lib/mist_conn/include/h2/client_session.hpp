#ifndef __MIST_INCLUDE_H2_CLIENT_SESSION_HPP__
#define __MIST_INCLUDE_H2_CLIENT_SESSION_HPP__

#include <memory>

#include "io/socket.hpp"

#include "h2/session.hpp"
#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class ClientRequest;
class ClientSessionImpl;

class ClientSession : public Session
{
public:

  ClientRequest submitRequest(std::string method, std::string path,
    std::string scheme, std::string authority, header_map headers,
    generator_callback cb = nullptr);

  virtual void shutdown() override;

  virtual void start() override;

  virtual void stop() override;

  virtual bool isStopped() const override;

  virtual void setOnError(error_callback cb) override;

  ClientSession(std::shared_ptr<io::Socket> socket);
  ClientSession(std::shared_ptr<ClientSessionImpl> impl);

  ClientSession(const ClientSession&) = default;
  ClientSession& operator=(const ClientSession&) = default;

  inline bool operator==(const ClientSession& rhs) const
    { return _impl == rhs._impl; }
  inline bool operator<(const ClientSession& rhs) const
    { return _impl < rhs._impl; }

  std::shared_ptr<ClientSessionImpl> _impl;

};

} // namespace h2
} // namespace mist

#endif
