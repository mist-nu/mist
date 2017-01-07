#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/utility/string_ref.hpp> 
#include <boost/variant.hpp>

#include <nss.h>
#include <cert.h>
#include <pk11priv.h>
#include <pk11pub.h>
#include <prthread.h>

#include "error/mist.hpp"
#include "error/nghttp2.hpp"
#include "error/nss.hpp"
#include "memory/nghttp2.hpp"
#include "memory/nss.hpp"

#include "io/address.hpp"
#include "io/ssl_context.hpp"
#include "io/ssl_socket.hpp"

#include "conn_impl.hpp"
#include "conn/peer.hpp"

#include "crypto/hash.hpp"

#include "nss/nss_base.hpp"
#include "nss/pubkey.hpp"

#include "tor/tor.hpp"

#include "h2/client_request.hpp"
#include "h2/client_response.hpp"
#include "h2/client_session.hpp"
#include "h2/client_stream.hpp"
#include "h2/server_request.hpp"
#include "h2/server_response.hpp"
#include "h2/server_session.hpp"
#include "h2/server_stream.hpp"
#include "h2/websocket.hpp"

namespace mist
{

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace
{
std::string to_hex(uint8_t byte)
{
  static const char *digits = "0123456789abcdef";
  std::array<char, 2> text{digits[byte >> 4], digits[byte & 0xf]};
  return std::string(text.begin(), text.end());
}

template<typename It>
std::string to_hex(It begin, It end)
{
  std::string text;
  while (begin != end)
    text += to_hex(uint8_t(*(begin++)));
  return text;
}

std::string to_hex(SECItem *item)
{
  return to_hex((uint8_t *)item->data, (uint8_t *)(item->data + item->len));
}

std::string to_hex(std::string str)
{
  return to_hex((uint8_t *)str.data(), (uint8_t *)(str.data() + str.size()));
}

} // namespace

/*
 * PeerDb
 */
PeerDb::PeerDb(ConnectContextImpl& ctx)
  : _ctx(ctx)
{}

std::pair<Peer&, bool>
PeerDb::findOrCreate(c_unique_ptr<SECKEYPublicKey> key)
{
  std::lock_guard<std::mutex> lock(_peerMutex);
  auto derPubKey = nss::derEncodePubKey(key.get());
  auto pubKeyHash = crypto::hash_sha3_256()->finalize(derPubKey.data(),
    derPubKey.data() + derPubKey.size());
  auto it = _peers.find(pubKeyHash);
  if (it != _peers.end()) {
    return std::pair<Peer&, bool>(it->second->facade(), false);
  } else {
    auto derPubKey = nss::derEncodePubKey(key.get());
    auto peer = std::unique_ptr<PeerImpl>(new PeerImpl(_ctx, derPubKey));
    assert(pubKeyHash == peer->publicKeyHash());
    auto peerIt = _peers.emplace(std::make_pair(pubKeyHash,
      std::move(peer)));
    assert(peerIt.second);
    return std::pair<Peer&, bool>(peerIt.first->second->facade(), true);
  }
}

void
PeerDb::remove(Peer& peer)
{
  /* We never remove authenticated peers */
  assert(!peer.authenticated());
  _peers.erase(peer.publicKeyHash());
}

/*
 * ConnectContext
 */
MistConnApi
ConnectContext::ConnectContext(io::SSLContext& sslCtx,
  authenticate_peer_callback cb)
  : _impl(std::make_shared<ConnectContextImpl>(sslCtx, std::move(cb)))
{}

MistConnApi
io::IOContext&
ConnectContext::ioCtx()
{
  return _impl->ioCtx();
}

MistConnApi
io::SSLContext&
ConnectContext::sslCtx()
{
  return _impl->sslCtx();
}

MistConnApi
void
ConnectContext::connectPeerDirect(Peer& peer, const io::Address& addr,
  connect_peer_callback cb)
{
  _impl->connectPeerDirect(peer, addr, cb);
}

MistConnApi
void
ConnectContext::connectPeerTor(Peer& peer, const PeerAddress& addr,
  connect_peer_callback cb)
{
  _impl->connectPeerTor(peer, addr, cb);
}

MistConnApi
Peer&
ConnectContext::addAuthenticatedPeer(
  const std::vector<std::uint8_t>& derPublicKey)
{
  return _impl->addAuthenticatedPeer(derPublicKey);
}

MistConnApi
std::uint16_t
ConnectContext::serveDirect(io::port_range_list directIncomingPort)
{
  return _impl->serveDirect(directIncomingPort);
}

MistConnApi
void
ConnectContext::startServeTor(io::port_range_list torIncomingPort,
  io::port_range_list torOutgoingPort, io::port_range_list controlPort,
  std::string executableName, std::string workingDir,
  tor_start_callback startCb, tor_exit_callback exitCb)
{
  _impl->startServeTor(torIncomingPort, torOutgoingPort, controlPort,
    std::move(executableName), std::move(workingDir), std::move(startCb),
    std::move(exitCb));
}

MistConnApi
void
ConnectContext::onionAddress(std::function<void(const std::string&)> cb)
{
  _impl->onionAddress(std::move(cb));
}

MistConnApi
void
ConnectContext::exec()
{
  _impl->exec();
}

MistConnApi
Service&
ConnectContext::newService(std::string name)
{
  return _impl->newService(std::move(name));
}

MistConnApi
std::uint16_t
ConnectContext::directConnectPort() const
{
  return _impl->directConnectPort();
}

MistConnApi
void
ConnectContext::openSSLSocket(const io::Address& addr,
    std::function<void(std::shared_ptr<io::Socket>,
        boost::system::error_code)> cb)
{
    _impl->openSSLSocket(addr, std::move(cb));
}

MistConnApi
void
ConnectContext::submitRequest(const io::Address& addr,
    std::string method, std::string path, std::string authority,
    mist::h2::header_map headers,
    std::function<void(boost::optional<h2::ClientRequest>,
        boost::system::error_code)> cb)
{
    _impl->submitRequest(addr, std::move(method), std::move(path),
        std::move(authority), std::move(headers), std::move(cb));
}

/*
 * ConnectContextImpl
 */

ConnectContextImpl::ConnectContextImpl(io::SSLContext& sslCtx,
  authenticate_peer_callback cb)
  : _sslCtx(sslCtx), _peerDb(*this), _authenticatePeerCb(std::move(cb))
{}

io::IOContext&
ConnectContextImpl::ioCtx()
{
  return _sslCtx.ioCtx();
}

io::SSLContext&
ConnectContextImpl::sslCtx()
{
  return _sslCtx;
}

Peer&
ConnectContextImpl::addAuthenticatedPeer(
  const std::vector<std::uint8_t>& derPublicKey)
{
  auto rv = _peerDb.findOrCreate(nss::decodeDerPublicKey(derPublicKey));
  Peer& peer = rv.first;
  bool created = rv.second;

  if (created) {
    /* The peer was created; mark it as authenticated */
    peer.setAuthenticated(true);
  } else {
    /* The peer already exists. We need to make a call to authenticated() to
       wait for the callback to authenticate the peer. */
    bool authenticated = peer.authenticated();

    /* We assume coherency between all calls to addAuthenticatedPeer
       and the authentication callback */
    if (!authenticated)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "addAuthenticatedPeer was called with a key that was marked as"
        " non-authenticated by the authenticate callback"));
  }

  return peer;
}

void
ConnectContextImpl::handshakePeer(io::SSLSocket& socket,
  boost::optional<Peer&> knownPeer,
  handshake_peer_callback cb)
{
  /* Trick to pass the authenticated peer to the handshake done callback */
  std::shared_ptr<boost::optional<Peer&>> peerRef
    = std::make_shared<boost::optional<Peer&>>(boost::none);
  
  socket.handshake(
    /* Handshake done */
    [this, peerRef, cb]
    (boost::system::error_code ec)
  {
    if (!ec && *peerRef)
      cb(**peerRef, boost::system::error_code());
    else
      cb(boost::none, ec);
  },
    /* Authenticate peer */
    [this, peerRef, knownPeer]
    (CERTCertificate* cert) -> bool
  {
    auto rv = _peerDb.findOrCreate(nss::certPubKey(cert));
    Peer& peer = rv.first;
    bool created = rv.second;

    if (created) {
      /* Delegate authentication to the authentication callback */
      _authenticatePeerCb(peer);
    }

    /* Blocking call for the authentication result */
    bool authenticated = peer.authenticated();

    if (created && !authenticated) {
      /* If we are in the thread that created the peer and it failed
         to authenticate, we are responsible for its removal */
      _peerDb.remove(peer);
    }

    if (authenticated) {
      /* Pass the peer reference to the handshake callback above */
      *peerRef = peer;
    } else {
      return false;
    }

    /* If we have a target peer, then compare pointers for equivalence */
    return !knownPeer || &peer == &*knownPeer;
  });
}

void
ConnectContextImpl::connectPeerDirect(Peer& peer, const io::Address& addr,
  connect_peer_callback cb)
{
  std::shared_ptr<io::SSLSocket> socket = _sslCtx.openSocket();
  socket->connect(addr,
    [this, &peer, socket, cb]
    (boost::system::error_code ec)
  {
    if (!ec) {
      handshakePeer(*socket, peer,
        [=, &peer]
        (boost::optional<Peer&>, boost::system::error_code ec)
      {
        if (!ec) {
          peer._impl.connection(std::move(socket),
            Peer::ConnectionType::Direct,
            Peer::ConnectionDirection::Client);
          cb(peer, boost::system::error_code());
        } else {
          /* Handshake failed */
          cb(peer, ec);
        }
      });
    } else {
      /* Connection failed */
      cb(peer, ec);
    }
  });
}

void
ConnectContextImpl::connectPeerTor(Peer& peer, const PeerAddress& addr,
  connect_peer_callback cb)
{
  std::shared_ptr<io::SSLSocket> socket = _sslCtx.openSocket();
  _torCtrl->connect(*socket, addr.hostname, addr.port,
    [=, &peer]
    (boost::system::error_code ec)
  {
    if (!ec) {
      handshakePeer(*socket, peer,
        [=, &peer]
        (boost::optional<Peer&>, boost::system::error_code ec)
      {
        if (!ec) {
          peer._impl.connection(std::move(socket),
            Peer::ConnectionType::Tor,
            Peer::ConnectionDirection::Client);
          cb(peer, boost::system::error_code());
        } else {
          /* Handshake failed */
          cb(peer, ec);
        }
      });
    } else {
      /* Connection failed */
      cb(peer, ec);
    }
  });
}

void
ConnectContextImpl::incomingDirectConnection(
  std::shared_ptr<io::SSLSocket> socket)
{
  std::cerr << "New direct connection!" << std::endl;
  
  handshakePeer(*socket, boost::none,
    [socket]
    (boost::optional<Peer&> peer, boost::system::error_code ec)
  {
    if (!ec) {
      peer->_impl.connection(std::move(socket), Peer::ConnectionType::Direct,
        Peer::ConnectionDirection::Server);
    } else {
      /* Handshake error, we cannot accept this connection */
      std::cerr << "Handshake error : " << ec.message() <<std::endl;
    }
  });
}

void
ConnectContextImpl::incomingTorConnection(
  std::shared_ptr<io::SSLSocket> socket)
{
  std::cerr << "New Tor connection!" << std::endl;
  
  handshakePeer(*socket, boost::none,
    [socket]
    (boost::optional<Peer&> peer, boost::system::error_code ec)
  {
    if (!ec) {
      peer->_impl.connection(socket, Peer::ConnectionType::Tor,
        Peer::ConnectionDirection::Server);
    } else {
      /* Handshake error, we cannot accept this connection */
      std::cerr << "Handshake error : " << ec.message() <<std::endl;
      return;
    }
  });
}

void
ConnectContextImpl::openSSLSocket(const io::Address& addr,
  std::function<void(std::shared_ptr<io::Socket>,
      boost::system::error_code)> cb)
{
  std::shared_ptr<io::SSLSocket> socket = _sslCtx.openSocket();
  socket->connect(addr,
    [cb, socket](boost::system::error_code ec)
  {
    if (!ec) {
      socket->handshake(
        /* Handshake done */
        [cb, socket](boost::system::error_code ec)
      {
        if (ec)
          cb(nullptr, ec);
        else
          cb(socket, boost::system::error_code());
      },
        /* Authenticate  */
        [](CERTCertificate*) -> bool { return true;  });
    } else {
        cb(nullptr, ec);
    }
  });
}

void
ConnectContextImpl::submitRequest(const io::Address& addr,
    std::string method, std::string path, std::string authority,
    mist::h2::header_map headers,
    std::function<void(boost::optional<h2::ClientRequest>,
      boost::system::error_code)> cb)
{
  openSSLSocket(addr,
      [=](std::shared_ptr<io::Socket> socket, boost::system::error_code ec)
  {
      if (!ec) {
          auto session = h2::ClientSession(std::move(socket));
          auto request = session.submitRequest(method, path, "https",
              authority, headers, nullptr);
          cb(request, boost::system::error_code());
      } else {
          cb(boost::none, ec);
      }
  });
}

std::uint16_t
ConnectContextImpl::serveDirect(io::port_range_list listenDirectPort)
{
  /* Serve the direct connection port */
  {
    using namespace std::placeholders;
    _directPort = sslCtx().serve(listenDirectPort,
      std::bind(&ConnectContextImpl::incomingDirectConnection, this, _1));
  }

  return _directPort;
}

std::uint16_t
ConnectContextImpl::directConnectPort() const
{
  return _directPort;
}

void
ConnectContextImpl::startServeTor(io::port_range_list torIncomingPort,
  io::port_range_list torOutgoingPort, io::port_range_list controlPort,
  std::string executableName, std::string workingDir,
  tor_start_callback startCb, tor_exit_callback exitCb)
{
  /* Serve the Tor connection port */
  {
    using namespace std::placeholders;
    _torIncomingPort = sslCtx().serve(torIncomingPort,
      std::bind(&ConnectContextImpl::incomingTorConnection, this, _1));
  }

  /* Start Tor */
  {
    _torCtrl = std::make_shared<tor::TorController>(ioCtx(), executableName,
      workingDir);
    _torHiddenService
      = _torCtrl->addHiddenService(_torIncomingPort, "mist-service");
    _torCtrl->start(torOutgoingPort, controlPort, startCb, exitCb);
  }
}

void
ConnectContextImpl::onionAddress(std::function<void(const std::string&)> cb)
{
  _torHiddenService->onionAddress(std::move(cb));
}

void
ConnectContextImpl::exec()
{
  ioCtx().exec();
}

Service&
ConnectContextImpl::newService(std::string name)
{
  auto service = std::make_shared<ServiceImpl>(*this, name);
  auto it = _services.emplace(std::make_pair(name, service));
  assert(it.second); // Assert insertion
  return service->facade();
}

void
ConnectContextImpl::onPeerError(Peer& peer, boost::system::error_code ec)
{
    std::cerr << "onPeerError " << ec.message() << std::endl;
}

void
ConnectContextImpl::onPeerRequest(Peer& peer, h2::ServerRequest request)
{
  if (!request.path() || !request.scheme()) {
    request.stream().close(boost::system::error_code());
    return;
  }

  auto& path = *request.path();
  auto& scheme = *request.scheme();

  assert(path[0] == '/');
  if (path == "/") {
    // Root
  } else if (path == "/mist/reverse") {
    if (scheme == "wss") {
      auto websocket
        = std::make_shared<mist::h2::ServerWebSocket>();
      websocket->start(request);
      peer._impl.reverseConnection(websocket,
        mist::Peer::ConnectionDirection::Client);
    } else {
      std::cerr << "Unrecognized reverse scheme " << scheme << std::endl;
      request.stream().close(boost::system::error_code());
    }
  } else {
    auto slashPos = path.find_first_of('/', 1);
    std::string rootDirName;
    if (slashPos == std::string::npos) {
      rootDirName = path.substr(1);
    } else {
      rootDirName = path.substr(1, slashPos - 1);
    }

    // TODO: Check root dir name for special stuff

    {
      auto serviceIt = _services.find(rootDirName);
      if (serviceIt != _services.end()) {
        std::string subPath = path.substr(slashPos + 1);
        if (scheme == "https") {
          serviceIt->second->onRequest(peer, request, subPath);
        } else if (scheme == "wss") {
          auto websocket
            = std::make_shared<mist::h2::ServerWebSocket>();
          websocket->start(request);
          serviceIt->second->onWebSocket(peer, subPath, websocket);
        } else {
          std::cerr << "Unrecognized scheme " << scheme << std::endl;
          request.stream().close(boost::system::error_code());
        }
      }
    }
  }
}

void
ConnectContextImpl::initializeReverseConnection(Peer& peer)
{
  assert(peer._impl._clientSession);
  mist::h2::ClientSession& session = *peer._impl._clientSession;

  auto websocket = std::make_shared<h2::ClientWebSocket>();
  websocket->start(session, "mist", "/mist/reverse");
  peer._impl.reverseConnection(websocket,
    mist::Peer::ConnectionDirection::Server);
}

void
ConnectContextImpl::onPeerConnectionStatus(Peer& peer,
  Peer::ConnectionStatus status)
{
  for (auto& service : _services) {
    service.second->onStatus(peer, status);
  }
}

void
ConnectContextImpl::serviceSubmit(Service& service, Peer& peer,
  std::string method, std::string path, Service::peer_submit_callback cb)
{
  mist::h2::ClientSession& session = *peer._impl._clientSession;

  mist::h2::ClientRequest request = session.submitRequest(std::move(method),
    "/" + service._impl._name + "/" + path, "https", "mist",
    mist::h2::header_map());
  cb(peer, request);
}

void
ConnectContextImpl::serviceOpenWebSocket(Service& service, Peer& peer,
  std::string path, Service::peer_websocket_callback cb)
{
  mist::h2::ClientSession& session = *peer._impl._clientSession;

  auto websocket = std::make_shared<h2::ClientWebSocket>();
  websocket->start(session, "mist", "/" + service._impl._name + "/" + path);
  cb(peer, path, websocket);
}

} // namespace mist
