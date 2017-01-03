/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <memory>
#include <string>

#include "conn/peer.hpp"

#include "io/socket.hpp"

#include "h2/server_request.hpp"
#include "h2/client_request.hpp"

namespace mist
{

class ConnectContext;
class ServiceImpl;

class MistConnApi Service
{
public:

  using peer_connection_status_callback
    = std::function<void(Peer&, Peer::ConnectionStatus)>;

  using peer_request_callback
    = std::function<void(Peer&, h2::ServerRequest, std::string)>;

  using peer_websocket_callback
    = std::function<void(Peer&, std::string, std::shared_ptr<io::Socket>)>;

  using peer_submit_callback
    = std::function<void(Peer&, h2::ClientRequest)>;

  void setOnPeerConnectionStatus(peer_connection_status_callback cb);

  void setOnPeerRequest(peer_request_callback cb);

  void submitRequest(Peer& peer, std::string method, std::string path,
    peer_submit_callback cb);

  void setOnWebSocket(peer_websocket_callback cb);

  void openWebSocket(Peer& peer, std::string path,
    peer_websocket_callback cb);

  Service(ServiceImpl& impl);

  ServiceImpl& _impl;

private:

  /* Forbid copy constructor and copy operator*/
  Service(Service&) = delete;
  Service& operator=(Service&) = delete;

};

} // namespace mist
