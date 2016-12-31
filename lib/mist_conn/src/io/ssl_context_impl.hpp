/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_SRC_IO_SSL_CONTEXT_IMPL_HPP__
#define __MIST_SRC_IO_SSL_CONTEXT_IMPL_HPP__

#include <cstddef>
#include <string>
#include <memory>
#include <list>

#include <prtypes.h>
#include <prio.h>
#include <pk11priv.h>
#include <pk11pub.h>

#include <nss.h>
#include <ssl.h>
#include <cert.h>

#include <boost/optional.hpp>

#include "memory/nss.hpp"

namespace mist
{
namespace io
{

class IOContext;
class SSLSocket;
class SSLContext;

class SSLContextImpl
{
public:

  using connection_callback = std::function<void(std::shared_ptr<SSLSocket>)>;

  IOContext& ioCtx();

  void serve(std::uint16_t servPort, connection_callback cb);
  std::uint16_t serve(port_range_list servPort, connection_callback cb);

  std::shared_ptr<SSLSocket> openSocket();

  /* NSS key and certificate interface */

  std::vector<std::uint8_t> derPublicKey() const;

  std::vector<std::uint8_t> publicKeyHash() const;

  std::vector<std::uint8_t> sign(const std::uint8_t* hashData,
    std::size_t length) const;

  bool verify(const std::vector<std::uint8_t>& derPublicKey,
	      const std::uint8_t* hashData,
	      std::size_t hashLength,
	      const std::uint8_t* signData,
	      std::size_t signLength) const;

private:

  friend class SSLSocket;
  friend class SSLContext;

  SSLContextImpl(IOContext& ioCtx, SSLContext & facade);

  SSLContextImpl(IOContext& ioCtx, const std::string& dbdir,
    SSLContext& facade);

  SSLContext& _facade;

  IOContext& _ioCtx;

  std::string _nickname;

  mutable c_unique_ptr<CERTCertificate> _certificate;

  CERTCertificate* certificate() const;

  mutable c_unique_ptr<SECKEYPrivateKey> _privateKey;

  SECKEYPrivateKey* privateKey() const;

  /* Upgrades the NSPR socket file descriptor to TLS */
  void initializeSecurity(c_unique_ptr<PRFileDesc>& fd);
  
  /* Initialize the socket with mist TLS settings */
  void initializeTLS(SSLSocket& sock);

  /* Called when NSS wants to get the client certificate */
  SECStatus getClientCert(SSLSocket& socket, CERTDistNames* caNames,
                          CERTCertificate** pRetCert,
                          SECKEYPrivateKey** pRetKy);

  /* Called when NSS wants to authenticate the peer certificate */
  SECStatus authCertificate(SSLSocket& socket, PRBool checkSig,
                            PRBool isServer);

};

} // namespace io
} // namespace mist

#endif
