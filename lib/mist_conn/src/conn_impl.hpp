/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_SRC_CONN_CONN_IMPL_HPP__
#define __MIST_SRC_CONN_CONN_IMPL_HPP__

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/variant.hpp>

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

#include "conn.hpp"

#include "peer_impl.hpp"

namespace mist
{

class ConnectContext;

class PeerDb
{
private:

  std::mutex _peerMutex;;
  std::map<std::string, std::shared_ptr<Peer>> _peers;

  ConnectContext& _ctx;

public:

  PeerDb(ConnectContext& ctx);

  std::pair<std::shared_ptr<Peer>, bool>
    findOrCreate(c_unique_ptr<SECKEYPublicKey> key);

  void remove(std::shared_ptr<Peer> peer);

};

class Service : public std::enable_shared_from_this<Service>
{
public:

  using peer_connection_status_callback =
    std::function<void(Peer&, Peer::ConnectionStatus)>;

  using peer_request_callback =
    std::function<void(Peer&, h2::ServerRequest&, std::string)>;

  using peer_websocket_callback =
    std::function<void(Peer&, std::string, std::shared_ptr<io::Socket>)>;

  using peer_submit_callback =
    std::function<void(Peer&, h2::ClientRequest&)>;

  void setOnPeerConnectionStatus(peer_connection_status_callback cb);

  void setOnPeerRequest(peer_request_callback cb);

  void submit(Peer& peer, std::string method, std::string path,
    peer_submit_callback cb);

  void setOnWebSocket(peer_websocket_callback cb);

  void openWebSocket(Peer& peer, std::string path,
    peer_websocket_callback cb);

  Service(ConnectContext& ctx, std::string name);

private:

  friend class ConnectContext;

  Service(Service&) = delete;
  Service& operator=(Service&) = delete;

  ConnectContext& _ctx;

  std::string _name;

  peer_connection_status_callback _onStatus;
  void onStatus(Peer& peer, Peer::ConnectionStatus status);

  peer_request_callback _onRequest;
  void onRequest(Peer& peer, h2::ServerRequest& request,
    std::string subPath);

  peer_websocket_callback _onWebSocket;
  void onWebSocket(Peer& peer, std::string path,
    std::shared_ptr<io::Socket> socket);

};

class ConnectContext
{
protected:

  using handshake_peer_callback
    = std::function<void(boost::optional<Peer&>, boost::system::error_code)>;

  void handshakePeer(io::SSLSocket& sock, boost::optional<Peer&> knownPeer,
    handshake_peer_callback cb);

  void incomingDirectConnection(std::shared_ptr<io::SSLSocket> socket);

  void incomingTorConnection(std::shared_ptr<io::SSLSocket> socket);

  void tryConnectPeerTor(Peer& peer, Peer::address_list::const_iterator it);

public:

  using authenticate_peer_callback = std::function<void(Peer&)>;

  ConnectContext(io::SSLContext& sslCtx, authenticate_peer_callback cb);

  void connectPeerDirect(Peer& peer, PRNetAddr* addr);

  void connectPeerTor(Peer& peer);

  io::IOContext& ioCtx();

  io::SSLContext& sslCtx();

  Peer& addAuthenticatedPeer(const std::string& derPublicKey);

  void serveDirect(std::uint16_t directIncomingPort);

  void startServeTor(std::uint16_t torIncomingPort,
                     std::uint16_t torOutgoingPort,
                     std::uint16_t controlPort,
                     std::string executableName,
                     std::string workingDir);

  void onionAddress(std::function<void(const std::string&)> cb);

  void exec();

  std::shared_ptr<Service> newService(std::string name);

private:

  friend class Peer;

  friend class Service;

  /* SSL context */
  io::SSLContext &_sslCtx;

  /* Tor controller */
  std::shared_ptr<tor::TorController> _torCtrl;

  /* Peer database */
  PeerDb _peerDb;

  /* Hidden service for incoming Tor connections */
  boost::optional<tor::TorHiddenService&> _torHiddenService;

  authenticate_peer_callback _authenticatePeerCb;

  std::map<std::string, std::shared_ptr<Service>> _services;

  void onPeerRequest(Peer& peer, h2::ServerRequest& request);

  void onPeerConnectionStatus(Peer& peer, Peer::ConnectionStatus status);

  void initializeReverseConnection(Peer& peer);

  void serviceSubmit(Service& service, Peer& peer, std::string method,
    std::string path, Service::peer_submit_callback cb);

  void serviceOpenWebSocket(Service& service, Peer& peer, std::string path,
    Service::peer_websocket_callback cb);
};

} // namespace mist

#endif
