/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <list>

#include <base64.h>

#include <prtypes.h>
#include <prio.h>
#include <pk11func.h>
#include <pk11priv.h>
#include <pk11pub.h>
#include <pkcs12.h>
#include <p12.h>
#include <p12plcy.h>

#include <nss.h>
#include <ssl.h>
#include <sslproto.h>
#include <cert.h>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/random/random_device.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include "crypto/hash.hpp"
#include "crypto/key.hpp"

#include "error/mist.hpp"
#include "error/nss.hpp"

#include "memory/nss.hpp"

#include "nss/nss_base.hpp"
#include "nss/pubkey.hpp"

#include "io/io_context.hpp"
#include "io/io_context_impl.hpp"
#include "io/rdv_socket.hpp"
#include "io/ssl_context.hpp"
#include "io/ssl_context_impl.hpp"
#include "io/ssl_socket.hpp"

namespace mist
{
namespace io
{
/*
 * SSLContext
 */
SSLContext::SSLContext(IOContext& ioCtx, const std::string& dbdir)
  : _impl(std::unique_ptr<SSLContextImpl>(
      new SSLContextImpl(ioCtx, dbdir, *this)))
{}

SSLContext::SSLContext(IOContext& ioCtx)
  : _impl(std::unique_ptr<SSLContextImpl>(new SSLContextImpl(ioCtx, *this)))
{}

SSLContext::~SSLContext()
{}

void 
SSLContext::loadPKCS12(const std::string& data, const std::string& password)
{
  nss::importPKCS12(_impl->_nickname, data, password);
}

void
SSLContext::loadPKCS12File(const std::string& path,
  const std::string& password)
{
  nss::importPKCS12File(_impl->_nickname, path, password);
}

void
SSLContext::serve(std::uint16_t servPort, connection_callback cb)
{
  _impl->serve(servPort, std::move(cb));
}

std::uint16_t
SSLContext::serve(port_range_list servPort, connection_callback cb)
{
  return _impl->serve(std::move(servPort), std::move(cb));
}

std::shared_ptr<SSLSocket>
SSLContext::openSocket()
{
  return _impl->openSocket();
}

IOContext& 
SSLContext::ioCtx()
{
  return _impl->ioCtx();
}

std::vector<std::uint8_t>
SSLContext::derPublicKey() const
{
  return _impl->derPublicKey();
}

std::vector<std::uint8_t>
SSLContext::publicKeyHash() const
{
  return _impl->publicKeyHash();
}


std::vector<std::uint8_t>
SSLContext::sign(const std::uint8_t * hashData, std::size_t length) const
{
  return _impl->sign(hashData, length);
}

bool
SSLContext::verify(const std::vector<std::uint8_t>& derPublicKey,
		   const std::uint8_t* hashData,
		   std::size_t hashLength,
		   const std::uint8_t* signData,
		   std::size_t signLength) const
{
  return _impl->verify(derPublicKey, hashData, hashLength,
		       signData, signLength);
}

/*
 * SSLContextImpl
 */
SSLContextImpl::SSLContextImpl(IOContext& ioCtx, SSLContext& facade)
  : _ioCtx(ioCtx), _facade(facade)
{
  _nickname = "mist" + std::to_string(nss::makeInstanceIndex());
}

SSLContextImpl::SSLContextImpl(IOContext& ioCtx, const std::string& dbdir,
  SSLContext& facade)
  : SSLContextImpl(ioCtx, facade)
{
  nss::initializeNSS(dbdir);
}

IOContext&
SSLContextImpl::ioCtx()
{
  return _ioCtx;
}

namespace
{

/* Try to get the certificate and private key with the given nickname */
std::pair<c_unique_ptr<CERTCertificate>,
          c_unique_ptr<SECKEYPrivateKey>>
getAuthData(const std::string& nickname, void* wincx)
{
  if (!nickname.empty()) {
    auto cert = to_unique(CERT_FindUserCertByUsage(
      CERT_GetDefaultCertDB(), const_cast<char *>(nickname.c_str()),
      certUsageSSLClient, PR_FALSE, wincx));
      
    if (!cert)
      return std::make_pair(to_unique<CERTCertificate>(),
                            to_unique<SECKEYPrivateKey>());
    
    auto privKey = to_unique(PK11_FindKeyByAnyCert(cert.get(), wincx));
    
    if (!privKey)
      return std::make_pair(to_unique<CERTCertificate>(),
                            to_unique<SECKEYPrivateKey>());
    
    return std::make_pair(std::move(cert), std::move(privKey));
  
  } else {
    /* No name given, automatically find the right cert. */
    auto names = to_unique(CERT_GetCertNicknames(CERT_GetDefaultCertDB(),
      SEC_CERT_NICKNAMES_USER, wincx));
      
    if (names) {
	
      for (std::size_t i = 0; i < names->numnicknames; ++i) {
        auto cert = to_unique(CERT_FindUserCertByUsage(
          CERT_GetDefaultCertDB(), names->nicknames[i], certUsageSSLClient,
          PR_FALSE, wincx));
        
        if (!cert)
          continue;
        
        if (CERT_CheckCertValidTimes(cert.get(), PR_Now(), PR_TRUE) !=
            secCertTimeValid)
          continue;
        
        auto privKey = to_unique(PK11_FindKeyByAnyCert(cert.get(), wincx));
        
        if (!privKey)
          continue;
    
        return std::make_pair(std::move(cert), std::move(privKey));
      }
    }
  }
  
  return std::make_pair(to_unique<CERTCertificate>(),
                        to_unique<SECKEYPrivateKey>());
}

} // namespace

/* Opens, binds a non-blocking SSL rendez-vous socket listening to the
   specified port. */
void
SSLContextImpl::serve(std::uint16_t servPort, connection_callback cb)
{
  const std::size_t backlog = 16;

  auto fd = openTCPSocket();
  {
    initializeSecurity(fd);
    
    /* Note that we do not call initializeTLS here, even though we
       theoretically could, since there is an issue with inheriting protocol
       negotiation settings */

    /* Set server certificate and private key */
    auto authData = getAuthData(_nickname, SSL_RevealPinArg(fd.get()));
    if (!authData.first || !authData.second)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_NO_KEY_OR_CERT),
        "Unable to find private key or certificate for rendez-vous socket"));

    if (SSL_ConfigSecureServer(fd.get(),
        authData.first.get(), authData.second.get(),
        NSS_FindCertKEAType(authData.first.get())) != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to set server certificate and key for rendez-vous socket"));

    /* Initialize addr to localhost:servPort */
    PRNetAddr addr;
    if (PR_InitializeNetAddr(PR_IpAddrLoopback, servPort, &addr) != PR_SUCCESS)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to initialize address for rendez-vous socket"));

    if (PR_Bind(fd.get(), &addr) != PR_SUCCESS)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_PORT_TAKEN),
        "Unable to bind rendez-vous socket to port "
        + std::to_string(servPort)));

    if (PR_Listen(fd.get(), backlog) != PR_SUCCESS)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to start listening to rendez-vous socket"));
  }

  _ioCtx.addDescriptor(
    std::make_shared<RdvSocket>(_facade, std::move(fd), std::move(cb)));
}

/* Opens, binds a non-blocking SSL rendez-vous socket listening to the
   first available port in the port list. */
std::uint16_t
SSLContextImpl::serve(port_range_list servPort, connection_callback cb)
{
  port_range_enumerator port(servPort);

  while (1) {
    if (port.at_end())
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_PORT_TAKEN),
        "Unable to bind socket to any port"));
    try {
      serve(port.get(), cb);
    } catch (const boost::system::system_error& ex) {
      if (ex.code().category() == mist_category()
        && ex.code().value() == MIST_ERR_PORT_TAKEN) {
        port.next();
        continue;
      } else {
        throw;
      }
    }
    break;
  }

  return port.get();
}

std::shared_ptr<SSLSocket>
SSLContextImpl::openSocket()
{
  auto socket = std::make_shared<SSLSocket>(_facade, openTCPSocket(), false);
  _ioCtx.addDescriptor(socket);
  return std::move(socket);
}

SECKEYPrivateKey*
SSLContextImpl::privateKey() const
{
  if (!_privateKey) {
    auto cert = certificate();
    _privateKey = to_unique(PK11_FindKeyByAnyCert(cert, nullptr));
    if (!_privateKey)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "SSL context private key not set"));
  }
  return _privateKey.get();
}

CERTCertificate*
SSLContextImpl::certificate() const
{
  if (!_certificate) {
    _certificate = to_unique(CERT_FindUserCertByUsage(
      CERT_GetDefaultCertDB(), const_cast<char*>(_nickname.c_str()),
      certUsageSSLClient, PR_FALSE, nullptr));
    //_certificate = to_unique(PK11_FindCertFromNickname(
    //  const_cast<char*>(_nickname.c_str()), nullptr));
    if (!_certificate)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "SSL context certificate not set"));
  }
  return _certificate.get();
}

/* Upgrades the NSPR socket file descriptor to TLS */
void
SSLContextImpl::initializeSecurity(c_unique_ptr<PRFileDesc>& fd)
{
  auto sslSockFd = to_unique(SSL_ImportFD(nullptr, fd.get()));
  if (!sslSockFd)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to wrap SSL socket"));

  /* We no longer own the old pointer */
  fd.release();
  
  fd = std::move(sslSockFd);

  /* All SSL sockets need SSL_SECURITY, enable it here. For sockets created
     by accepting a rendez-vous socket, this setting will be inherited */
  if (SSL_OptionSet(fd.get(), SSL_SECURITY, PR_TRUE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_SECURITY setting"));

  /* Set the PK11 user data to this ssl context */
  if (SSL_SetPKCS11PinArg(fd.get(), this) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to set PKCS11 Pin Arg"));
}

/* Called when NSS wants to get the client certificate */
SECStatus
SSLContextImpl::getClientCert(SSLSocket& socket, CERTDistNames* caNames,
  CERTCertificate** pRetCert, SECKEYPrivateKey** pRetKey)
{
  auto authData = getAuthData(_nickname, SSL_RevealPinArg(socket.fileDesc()));
  if (!authData.first || !authData.second) {
    /* Private key or certificate was not found */
    return SECFailure;
  } else {
    *pRetCert = authData.first.release();
    *pRetKey = authData.second.release();
    return SECSuccess;
  }
}

/* Called when NSS wants to authenticate the peer certificate */
SECStatus
SSLContextImpl::authCertificate(SSLSocket& socket, PRBool checkSig,
  PRBool isServer)
{
  auto cert = to_unique(SSL_PeerCertificate(socket.fileDesc()));
  auto pubKey = nss::derEncodeCertPubKey(cert.get());
  auto pubKeyHash = crypto::hash_sha3_256()->finalize(pubKey.data(),
    pubKey.data() + pubKey.size());

  /* Check certificate time validity */
  if (CERT_CheckCertValidTimes(cert.get(), PR_Now(), PR_TRUE)
      != secCertTimeValid)
    return SECFailure;
  
  if (!socket.authenticate(cert.get()))
    return SECFailure;
  
  return SECSuccess;
}

namespace
{
/* Returns an nghttp2-compatible protocols string */
std::vector<unsigned char>
h2Protocol()
{
  return std::vector<unsigned char>{ 2, 'h', '2' };
}
} // namespace

/* Initialize the SSL socket with mist TLS settings */
void
SSLContextImpl::initializeTLS(SSLSocket& sock)
{
  PRFileDesc* sslfd = sock.fileDesc();

  /* Server requests certificate from client */
  if (SSL_OptionSet(sslfd, SSL_REQUEST_CERTIFICATE, PR_TRUE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_REQUEST_CERTIFICATE option"));

  /* Require certificate */
  if (SSL_OptionSet(sslfd, SSL_REQUIRE_CERTIFICATE , PR_TRUE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_REQUIRE_CERTIFICATE option"));

  /* Disable SSLv3 */
  if (SSL_OptionSet(sslfd, SSL_ENABLE_SSL3, PR_FALSE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_ENABLE_SSL3 option"));

  /* Enable TLS */
  if (SSL_OptionSet(sslfd, SSL_ENABLE_TLS, PR_TRUE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_ENABLE_TLS option"));

  /* Require latest TLS version */
  // TODO: TLS v1.3
  {
    SSLVersionRange sslverrange = {
      SSL_LIBRARY_VERSION_TLS_1_2, SSL_LIBRARY_VERSION_TLS_1_2
    };
    if (SSL_VersionRangeSet(sslfd, &sslverrange) != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to set SSL version"));
  }

  /* Disable session cache */
  if (SSL_OptionSet(sslfd, SSL_NO_CACHE, PR_TRUE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_NO_CACHE option"));

  /* Enable ALPN */
  if (SSL_OptionSet(sslfd, SSL_ENABLE_ALPN, PR_TRUE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_ENABLE_NPN option"));

  /* Disable NPN */
  // TODO:
  if (SSL_OptionSet(sslfd, SSL_ENABLE_NPN, PR_TRUE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_ENABLE_ALPN option"));

  /* Set the only supported protocol to HTTP/2 */
  {
    auto protocols = h2Protocol();
    if (SSL_SetNextProtoNego(sslfd, protocols.data(),
        static_cast<unsigned int>(protocols.size())) != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to set protocol negotiation"));
  }

  /* Client certificate and key callback */
  {
    auto rv = SSL_AuthCertificateHook(sslfd,
    [](void* arg, PRFileDesc* fd, PRBool checkSig,
       PRBool isServer) -> SECStatus
    {
      SSLSocket &socket = *static_cast<SSLSocket*>(arg);
      return socket.sslCtx()._impl->authCertificate(socket, checkSig, isServer);
    }, &sock);

    if (rv != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to set AuthCertificateHook"));
  }

  /* Handshake as server */
  if (SSL_OptionSet(sslfd, SSL_HANDSHAKE_AS_SERVER, sock.isServer()
      ? PR_TRUE : PR_FALSE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_HANDSHAKE_AS_SERVER option"));
      
  /* Handshake as client */
  if (SSL_OptionSet(sslfd, SSL_HANDSHAKE_AS_CLIENT, sock.isServer()
      ? PR_FALSE : PR_TRUE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to modify SSL_HANDSHAKE_AS_CLIENT option"));

  if (!sock.isServer()) {
    /* Set client certificate and key callback */
    auto rv = SSL_GetClientAuthDataHook(sslfd,
    [](void* arg, PRFileDesc* fd, CERTDistNames* caNames,
       CERTCertificate** pRetCert, SECKEYPrivateKey** pRetKey) -> SECStatus
    {
      SSLSocket &socket = *static_cast<SSLSocket*>(arg);
      return socket.sslCtx()._impl->getClientCert(socket, caNames, pRetCert,
        pRetKey);
    }, &sock);
    
    if (rv != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to set GetClientAuthDataHook"));
  }

  /* Reset handshake */
  /* TODO: Check if necessary */
  if(SSL_ResetHandshake(sslfd, sock.isServer()
     ? PR_TRUE : PR_FALSE) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to reset handshake"));
}

std::vector<std::uint8_t>
SSLContextImpl::derPublicKey() const
{
  auto pubKey = to_unique(SECKEY_ConvertToPublicKey(privateKey()));
  return nss::derEncodePubKey(pubKey.get());
}

std::vector<std::uint8_t>
SSLContextImpl::publicKeyHash() const
{
  auto derPubKey = derPublicKey();
  return crypto::hash_sha3_256()->finalize(derPubKey.data(),
    derPubKey.data() + derPubKey.size());
}

std::vector<std::uint8_t>
SSLContextImpl::sign(const std::uint8_t * hashData, std::size_t length) const
{
  return nss::sign(privateKey(), hashData, length);
}

bool
SSLContextImpl::verify(const std::vector<std::uint8_t>& derPublicKey,
		       const std::uint8_t* hashData,
		       std::size_t hashLength,
		       const std::uint8_t* signData,
		       std::size_t signLength) const
{
  return nss::verify(nss::decodeDerPublicKey(derPublicKey).get(),
		     hashData, hashLength, signData, signLength);
}


} // namespace io
} // namespace mist
