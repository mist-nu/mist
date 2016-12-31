/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef __MIST_INCLUDE_CONN_CONN_HPP__
#define __MIST_INCLUDE_CONN_CONN_HPP__

#include <cstddef>
#include <memory>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include "io/io_context.hpp"
#include "io/ssl_context.hpp"
#include "h2/client_request.hpp"

namespace mist
{
namespace io
{
class Address;
class IOContext;
class Socket;
class SSLContext;
}

class Peer;
class Service;
class ConnectContextImpl;

class ConnectContext
{
public:

  using authenticate_peer_callback = std::function<void(Peer&)>;

  ConnectContext(io::SSLContext& sslCtx, authenticate_peer_callback cb);

  io::IOContext& ioCtx();

  io::SSLContext& sslCtx();

  boost::system::error_code connectPeerDirect(Peer& peer,
    const io::Address& addr);

  boost::system::error_code connectPeerTor(Peer& peer);

  Peer& addAuthenticatedPeer(const std::vector<std::uint8_t>& derPublicKey);

  std::uint16_t serveDirect(io::port_range_list directIncomingPort);

  void startServeTor(io::port_range_list torIncomingPort,
    io::port_range_list torOutgoingPort, io::port_range_list controlPort,
    std::string executableName, std::string workingDir);

  void onionAddress(std::function<void(const std::string&)> cb);

  void exec();

  Service& newService(std::string name);

  std::shared_ptr<ConnectContextImpl> _impl;

  std::uint16_t directConnectPort() const;

  void openSSLSocket(const io::Address& addr,
    std::function<void(std::shared_ptr<io::Socket>)> cb);

  void submitRequest(const io::Address& addr, std::string method,
    std::string path, std::function<void(boost::optional<h2::ClientRequest>)> cb);
};

} // namespace mist

#endif
