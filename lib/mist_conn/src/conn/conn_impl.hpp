#ifndef __MIST_HEADERS_CONN_HPP__
#define __MIST_HEADERS_CONN_HPP__

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/variant.hpp>

#include "conn/conn.hpp"
#include "conn/peer.hpp"
#include "conn/peer_impl.hpp"
#include "conn/service.hpp"
#include "conn/service_impl.hpp"

#include "error/mist.hpp"
#include "error/nghttp2.hpp"
#include "error/nss.hpp"
#include "memory/nghttp2.hpp"
#include "memory/nss.hpp"

#include "tor/tor.hpp"

#include "h2/session.hpp"
#include "h2/stream.hpp"
#include "h2/client_request.hpp"
#include "h2/client_response.hpp"
#include "h2/server_request.hpp"
#include "h2/server_response.hpp"

#include "io/io_context.hpp"
#include "io/ssl_context.hpp"
#include "io/ssl_socket.hpp"

#include "conn/conn.hpp"

namespace mist
{

class PeerDb
{
public:

  PeerDb(ConnectContextImpl& ctx);

  std::pair<Peer&, bool>
    findOrCreate(c_unique_ptr<SECKEYPublicKey> key);

  void remove(Peer& peer);

private:

  std::mutex _peerMutex;
  std::map<std::vector<std::uint8_t>, std::unique_ptr<PeerImpl>> _peers;

  ConnectContextImpl& _ctx;

};

class ConnectContextImpl
{
protected:

  using handshake_peer_callback
    = std::function<void(boost::optional<Peer&>, boost::system::error_code)>;

  void handshakePeer(io::SSLSocket& sock, boost::optional<Peer&> knownPeer,
    handshake_peer_callback cb);

  void incomingDirectConnection(std::shared_ptr<io::SSLSocket> socket);

  void incomingTorConnection(std::shared_ptr<io::SSLSocket> socket);

  //boost::system::error_code tryConnectPeerTor(Peer& peer,
  //  Peer::address_list::const_iterator it);

public:

  using connect_peer_callback
    = ConnectContext::connect_peer_callback;

  using authenticate_peer_callback
    = ConnectContext::authenticate_peer_callback;

  ConnectContextImpl(io::SSLContext& sslCtx, authenticate_peer_callback cb);

  void connectPeerDirect(Peer& peer,
    const io::Address& addr, connect_peer_callback cb);

  void connectPeerTor(Peer& peer,
    const PeerAddress& addr, connect_peer_callback cb);

  io::IOContext& ioCtx();

  io::SSLContext& sslCtx();

  Peer& addAuthenticatedPeer(const std::vector<std::uint8_t>& derPublicKey);

  std::uint16_t serveDirect(io::port_range_list directIncomingPort);

  std::uint16_t directConnectPort() const;

  void startServeTor(io::port_range_list torIncomingPort,
    io::port_range_list torOutgoingPort, io::port_range_list controlPort,
    std::string executableName, std::string workingDir);

  void onionAddress(std::function<void(const std::string&)> cb);

  void exec();

  Service& newService(std::string name);

  void openSSLSocket(const io::Address& addr,
    std::function<void(std::shared_ptr<io::Socket>,
        boost::system::error_code)> cb);

  void submitRequest(const io::Address& addr, std::string method,
    std::string path, std::string authority, mist::h2::header_map headers,
    std::function<void(boost::optional<h2::ClientRequest>,
      boost::system::error_code)> cb);

private:

  friend class PeerImpl;
  friend class ServiceImpl;

  /* SSL context */
  io::SSLContext& _sslCtx;

  /* Tor controller */
  std::shared_ptr<tor::TorController> _torCtrl;

  /* Peer database */
  PeerDb _peerDb;

  /* Hidden service for incoming Tor connections */
  boost::optional<tor::TorHiddenService&> _torHiddenService;

  /* Ports */
  std::uint16_t _directPort;
  std::uint16_t _torIncomingPort;
  std::uint16_t _torOutgoingPort;
  std::uint16_t _torControlPort;

  authenticate_peer_callback _authenticatePeerCb;

  std::map<std::string, std::shared_ptr<ServiceImpl>> _services;
  
  void onPeerRequest(Peer& peer, h2::ServerRequest request);

  void onPeerError(Peer& peer, boost::system::error_code ec);

  void onPeerConnectionStatus(Peer& peer, PeerImpl::ConnectionStatus status);

  void initializeReverseConnection(Peer& peer);

  void serviceSubmit(Service& service, Peer& peer, std::string method,
    std::string path, Service::peer_submit_callback cb);

  void serviceOpenWebSocket(Service& service, Peer& peer, std::string path,
    Service::peer_websocket_callback cb);
};

} // namespace mist

#endif
