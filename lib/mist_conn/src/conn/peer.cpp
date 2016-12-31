/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#include <memory>
#include <string>

#include "peer_impl.hpp"

#include "conn/conn_impl.hpp"
#include "conn/peer_impl.hpp"

#include "io/ssl_context_impl.hpp"

#include "tor/tor.hpp"

#include "crypto/hash.hpp"

#include "nss/nss_base.hpp"
#include "nss/pubkey.hpp"

#include "h2/client_session.hpp"
#include "h2/server_session.hpp"

namespace mist
{

/*
* Peer
*/
Peer::Peer(PeerImpl& impl)
  : _impl(impl)
{}

const std::vector<std::uint8_t>&
Peer::derPublicKey() const
{
  return _impl.derPublicKey();
}

const std::vector<std::uint8_t>&
Peer::publicKeyHash() const
{
  return _impl.publicKeyHash();
}

const Peer::address_list&
Peer::addresses() const
{
  return _impl.addresses();
}

void
Peer::addAddress(PeerAddress address)
{
  _impl.addAddress(address);
}

bool
Peer::authenticated() const
{
  return _impl.authenticated();
}

void
Peer::setAuthenticated(bool authenticated)
{
  _impl.setAuthenticated(authenticated);
}

bool
Peer::verify(const std::uint8_t* hashData, std::size_t hashLength,
  const std::uint8_t* signData, std::size_t signLength) const
{
  return _impl.verify(hashData, hashLength, signData, signLength);
}

/*
* PeerImpl
*/
PeerImpl::PeerImpl(ConnectContextImpl& ctx,
  const std::vector<std::uint8_t>& derPublicKey)
  : _ctx(ctx), _pubKey(nss::decodeDerPublicKey(derPublicKey)),
  _derPubKey(derPublicKey), _facade(*this), _friend(false), _nonfriend(false)
{
  _pubKeyHash = crypto::hash_sha3_256()->finalize(derPublicKey.data(),
    derPublicKey.data() + derPublicKey.size());
}

void
PeerImpl::connection(std::shared_ptr<io::Socket> socket,
  ConnectionType connType, ConnectionDirection connDirection)
{
  if (_socket) {
    /* TODO: Migrate to the new socket, if we accept this migration */
    std::cerr << "New connection to peer when already connected" << std::endl;
  }

  _socket = socket;
  if (connDirection == ConnectionDirection::Server) {
    /* TODO */
    assert(!_serverSession);
    using namespace std::placeholders;
    _serverSession = h2::ServerSession(socket);
    _serverSession->setOnRequest(std::bind(&ConnectContextImpl::onPeerRequest,
      &_ctx, std::ref(_facade), _1));
  } else {
    /* TODO */
    assert(!_clientSession);
    _clientSession = h2::ClientSession(socket);
    _ctx.initializeReverseConnection(_facade);
    _ctx.onPeerConnectionStatus(_facade, ConnectionStatus::Connected);
  }
}

void
PeerImpl::reverseConnection(std::shared_ptr<io::Socket> socket,
  ConnectionDirection connDirection)
{
  if (connDirection == ConnectionDirection::Server) {
    using namespace std::placeholders;
    _reverseServerSession = h2::ServerSession(socket);
    _reverseServerSession->setOnRequest(
      std::bind(&ConnectContextImpl::onPeerRequest, &_ctx,
        std::ref(_facade), _1));
  } else {
    _reverseClientSession = h2::ClientSession(socket);
    _ctx.onPeerConnectionStatus(_facade, ConnectionStatus::Connected);
  }
}

const SECKEYPublicKey*
PeerImpl::publicKey() const
{
  return _pubKey.get();
}

const std::vector<std::uint8_t>&
PeerImpl::derPublicKey() const
{
  return _derPubKey;
}

const std::vector<std::uint8_t>&
PeerImpl::publicKeyHash() const
{
  return _pubKeyHash;
}

const Peer::address_list&
PeerImpl::addresses() const
{
  return _addresses;
}

void
PeerImpl::addAddress(PeerAddress address)
{
  _addresses.push_back(std::move(address));
}

bool
PeerImpl::authenticated() const
{
  std::unique_lock<std::mutex> lck(_authenticateMutex);
  while (!_friend && !_nonfriend) _authenticateCV.wait(lck);
  return _friend;
}

void
PeerImpl::setAuthenticated(bool authenticated)
{
  std::unique_lock<std::mutex> lck(_authenticateMutex);
  assert(!_friend && !_nonfriend);
  if (authenticated) {
    _friend = true;
    _nonfriend = false;
  } else {
    _friend = false;
    _nonfriend = true;
  }
  _authenticateCV.notify_all();
}

bool
PeerImpl::verify(const std::uint8_t* hashData, std::size_t hashLength,
  const std::uint8_t* signData, std::size_t signLength) const
{
  return nss::verify(_pubKey.get(), hashData, hashLength,
    signData, signLength);
}

h2::ClientSession&
PeerImpl::clientSession()
{
  assert(_clientSession || _reverseClientSession);
  return _clientSession ? *_clientSession : *_reverseClientSession;
}

} // namespace mist
