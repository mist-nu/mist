/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#include "conn/conn_impl.hpp"
#include "conn/service_impl.hpp"

namespace mist
{

/*
* Service
*/
Service::Service(ServiceImpl& impl)
  : _impl(impl)
{}

void
Service::setOnPeerConnectionStatus(peer_connection_status_callback cb)
{
  _impl.setOnPeerConnectionStatus(std::move(cb));
}

void
Service::setOnPeerRequest(peer_request_callback cb)
{
  _impl.setOnPeerRequest(std::move(cb));
}

void
Service::submitRequest(Peer& peer, std::string method, std::string path,
  peer_submit_callback cb)
{
  _impl.submitRequest(peer, std::move(method),
    std::move(path), std::move(cb));
}

void
Service::setOnWebSocket(peer_websocket_callback cb)
{
  _impl.setOnWebSocket(std::move(cb));
}

void
Service::openWebSocket(Peer& peer, std::string path,
  peer_websocket_callback cb)
{
  _impl.openWebSocket(peer, std::move(path), std::move(cb));
}

/*
* ServiceImpl
*/
ServiceImpl::ServiceImpl(ConnectContextImpl& ctx, std::string name)
  : _ctx(ctx), _name(name), _facade(*this)
{}

void
ServiceImpl::setOnPeerConnectionStatus(peer_connection_status_callback cb)
{
  _onStatus = std::move(cb);
}

void
ServiceImpl::onStatus(Peer& peer, Peer::ConnectionStatus status)
{
  if (_onStatus)
    _onStatus(peer, status);
}

void
ServiceImpl::setOnPeerRequest(peer_request_callback cb)
{
  _onRequest = std::move(cb);
}

void
ServiceImpl::onRequest(Peer& peer, h2::ServerRequest request,
  std::string subPath)
{
  if (_onRequest)
    _onRequest(peer, std::move(request),
      std::move(subPath));
}

void
ServiceImpl::submitRequest(Peer& peer, std::string method,
  std::string path, peer_submit_callback cb)
{
  _ctx.serviceSubmit(_facade, peer, std::move(method), std::move(path),
    std::move(cb));
}

void
ServiceImpl::openWebSocket(Peer& peer, std::string path,
  peer_websocket_callback cb)
{
  _ctx.serviceOpenWebSocket(_facade, peer, std::move(path), std::move(cb));
}

void
ServiceImpl::setOnWebSocket(peer_websocket_callback cb)
{
  _onWebSocket = std::move(cb);
}

void
ServiceImpl::onWebSocket(Peer& peer, std::string path,
  std::shared_ptr<io::Socket> socket)
{
  if (_onWebSocket)
    _onWebSocket(peer, std::move(path), std::move(socket));
}

} // namespace mist
