/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef __MIST_HEADERS_CONN_PEER_HPP__
#define __MIST_HEADERS_CONN_PEER_HPP__

#include <cstddef>
#include <list>
#include <memory>
#include <vector>

namespace mist
{

class ConnectContext;
class PeerImpl;

struct PeerAddress
{
  std::string hostname;
  std::uint16_t port;
};

class Peer
{
public:

  using address_list = std::list<PeerAddress>;
  enum class ConnectionType { Direct, Tor };
  enum class ConnectionDirection { Client, Server };
  enum class ConnectionStatus { Disconnected, Connected };

  const std::vector<std::uint8_t>& derPublicKey() const;

  const std::vector<std::uint8_t>& publicKeyHash() const;

  const address_list& addresses() const;

  void addAddress(PeerAddress address);

  bool authenticated() const;
  void setAuthenticated(bool authenticated);

  bool verify(const std::uint8_t* hashData, std::size_t hashLength,
    const std::uint8_t* signData, std::size_t signLength) const;

  Peer(PeerImpl& impl);

  PeerImpl& _impl;

private:

  /* Forbid copy constructor and copy operator*/
  Peer(Peer&) = delete;
  Peer& operator=(Peer&) = delete;

};

} // namespace mist

#endif
