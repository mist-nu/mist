/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_SRC_CONN_SERVICE_IMPL_HPP__
#define __MIST_SRC_CONN_SERVICE_IMPL_HPP__

#include <cstddef>
#include <memory>
#include <string>

#include "conn/conn.hpp"
#include "conn/peer.hpp"
#include "conn/service.hpp"

#include "io/socket.hpp"

#include "h2/server_request.hpp"
#include "h2/client_request.hpp"

namespace mist
{

class ConnectContextImpl;

class ServiceImpl
{
public:

  using peer_connection_status_callback
    = Service::peer_connection_status_callback;

  using peer_request_callback
    = Service::peer_request_callback;

  using peer_websocket_callback
    = Service::peer_websocket_callback;

  using peer_submit_callback
    = Service::peer_submit_callback;

  void setOnPeerConnectionStatus(peer_connection_status_callback cb);

  void setOnPeerRequest(peer_request_callback cb);

  void submitRequest(Peer& peer, std::string method, std::string path,
    peer_submit_callback cb);

  void setOnWebSocket(peer_websocket_callback cb);

  void openWebSocket(Peer& peer, std::string path,
    peer_websocket_callback cb);

  ServiceImpl(ConnectContextImpl& ctx, std::string name);

  inline Service& facade() { return _facade; }

private:

  Service _facade;

  friend class ConnectContextImpl;

  ServiceImpl(ServiceImpl&) = delete;
  ServiceImpl& operator=(ServiceImpl&) = delete;

  std::string _name;

  ConnectContextImpl& _ctx;

  peer_connection_status_callback _onStatus;
  void onStatus(Peer& peer, Peer::ConnectionStatus status);

  peer_request_callback _onRequest;
  void onRequest(Peer& peer, h2::ServerRequest request,
    std::string subPath);

  peer_websocket_callback _onWebSocket;
  void onWebSocket(Peer& peer, std::string path,
    std::shared_ptr<io::Socket> socket);

};

} // namespace mist

#endif
