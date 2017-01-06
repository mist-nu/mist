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
struct PeerAddress;
class Service;
class ConnectContextImpl;

class MistConnApi ConnectContext
{
public:

  using connect_peer_callback = std::function<void(Peer&,
    boost::system::error_code)>;

  using authenticate_peer_callback = std::function<void(Peer&)>;

  ConnectContext(io::SSLContext& sslCtx, authenticate_peer_callback cb);

  io::IOContext& ioCtx();

  io::SSLContext& sslCtx();

  void connectPeerDirect(Peer& peer, const io::Address& addr,
    connect_peer_callback cb);

  void connectPeerTor(Peer& peer, const PeerAddress& addr,
    connect_peer_callback cb);

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
    std::function<void(std::shared_ptr<io::Socket>,
        boost::system::error_code)> cb);

  void submitRequest(const io::Address& addr, std::string method,
      std::string path, std::string authority, mist::h2::header_map headers,
      std::function<void(boost::optional<h2::ClientRequest>,
          boost::system::error_code)> cb);
};

} // namespace mist
