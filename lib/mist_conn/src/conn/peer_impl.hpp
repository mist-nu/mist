/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_SRC_CONN_PEER_IMPL_HPP__
#define __MIST_SRC_CONN_PEER_IMPL_HPP__

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/variant.hpp>

#include "conn/peer.hpp"

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
#include "h2/client_session.hpp"
#include "h2/server_request.hpp"
#include "h2/server_response.hpp"
#include "h2/server_session.hpp"

#include "io/io_context.hpp"
#include "io/ssl_context.hpp"
#include "io/ssl_socket.hpp"

namespace mist
{

class ConnectContextImpl;

class PeerImpl
{
public:

  using address_list = Peer::address_list;
  using ConnectionType = Peer::ConnectionType;
  using ConnectionDirection = Peer::ConnectionDirection;
  using ConnectionStatus = Peer::ConnectionStatus;

  void connection(std::shared_ptr<io::Socket> socket,
    ConnectionType connType, ConnectionDirection connDirection);

  void reverseConnection(std::shared_ptr<io::Socket> socket,
    ConnectionDirection connDirection);

  const SECKEYPublicKey* publicKey() const;

  const std::vector<std::uint8_t>& derPublicKey() const;

  const std::vector<std::uint8_t>& publicKeyHash() const;

  const address_list& addresses() const;

  void addAddress(PeerAddress address);

  bool authenticated() const;
  void setAuthenticated(bool authenticated);

  bool verify(const std::uint8_t* hashData, std::size_t hashLength,
    const std::uint8_t* signData, std::size_t signLength) const;

  PeerImpl(ConnectContextImpl& ctx,
    const std::vector<std::uint8_t>& derPublicKey);

  inline Peer& facade() { return _facade; }

private:

  friend class ConnectContextImpl;

  Peer _facade;

  h2::ClientSession& clientSession();

  std::shared_ptr<io::Socket> _socket;
  boost::optional<h2::ServerSession> _serverSession;
  boost::optional<h2::ClientSession> _clientSession;
  boost::optional<h2::ServerSession> _reverseServerSession;
  boost::optional<h2::ClientSession> _reverseClientSession;

  address_list _addresses;

  /* Peer public key */
  c_unique_ptr<SECKEYPublicKey> _pubKey;
  std::vector<std::uint8_t> _derPubKey;
  std::vector<std::uint8_t> _pubKeyHash;

  /* Thread-safe authentication */
  mutable std::mutex _authenticateMutex;
  mutable std::condition_variable _authenticateCV;
  bool _friend;
  bool _nonfriend;

  ConnectContextImpl& _ctx;
};

} // namespace mist

#endif
