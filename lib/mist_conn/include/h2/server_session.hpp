#ifndef __MIST_INCLUDE_H2_SERVER_SESSION_HPP__
#define __MIST_INCLUDE_H2_SERVER_SESSION_HPP__

#include <memory>

#include "io/socket.hpp"

#include "h2/session.hpp"
#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class ServerSessionImpl;

class ServerSession : public Session
{
public:

  void setOnRequest(server_request_callback cb);

  virtual void shutdown() override;

  virtual void start() override;

  virtual void stop() override;

  virtual bool isStopped() const override;

  virtual void setOnError(error_callback cb) override;

  ServerSession(std::shared_ptr<io::Socket> socket);
  ServerSession(std::shared_ptr<ServerSessionImpl> impl);

  ServerSession(const ServerSession&) = default;
  ServerSession& operator=(const ServerSession&) = default;

  inline bool operator==(const ServerSession& rhs) const
    { return _impl == rhs._impl; }
  inline bool operator<(const ServerSession& rhs) const
    { return _impl < rhs._impl; }

  std::shared_ptr<ServerSessionImpl> _impl;

};

} // namespace h2
} // namespace mist

#endif
