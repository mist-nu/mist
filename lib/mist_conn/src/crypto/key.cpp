#include "nspr.h"
#include "prtypes.h"
#include "prtime.h"
#include "prlong.h"

#include "pk11func.h"
#include "secasn1.h"
#include "cert.h"
#include "cryptohi.h"
#include "secoid.h"
#include "certdb.h"
#include "nss.h"

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include "crypto/key.hpp"

#include "memory/nss.hpp"

#include "error/nss.hpp"

#include "nss/nss_base.hpp"
#include "nss/pubkey.hpp"

#include "io/ssl_context.hpp"
#include "io/ssl_context_impl.hpp"

namespace mist
{
namespace crypto
{

namespace
{
c_unique_ptr<CERTCertificateRequest>
makeCertReq(SECKEYPublicKey *pubKey, CERTName* subject)
{
  /* Create info about public key */
  auto spki = to_unique(SECKEY_CreateSubjectPublicKeyInfo(pubKey));
  if (!spki)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
       "Unable to create subject public key"));

  /* Generate certificate request */
  auto cr = to_unique(
    CERT_CreateCertificateRequest(subject, spki.get(), nullptr));
  if (!cr)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to make certificate request"));

  /* Request attributes */
  /* TODO: Extensions */
  //const char* phone,
  //const char* emailAddrs,
  //const char* dnsNames,
  //certutilExtnList extnList,
  //const char *extGeneric,
  /*{
      void* extHandle = CERT_StartCertificateRequestAttributes(cr.get());
      if (!extHandle)
          BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
              "Error while calling CERT_StartCertificateRequestAttributes"));
      if (AddExtensions(extHandle, emailAddrs, dnsNames, extnList, extGeneric)
          != SECSuccess)
          BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
              "Unable to add extensions"));
      CERT_FinishExtensions(extHandle);
      CERT_FinishCertificateRequestAttributes(cr.get());
  }*/

  return cr;
}

c_unique_ptr<SECItem>
signCertReq(CERTCertificateRequest* cr, SECKEYPrivateKey* privKey,
  KeyType keyType, SECOidTag hashAlgTag, CERTName* subject)
{
  /* Request item  */
  auto signedReq = to_unique(SECITEM_AllocItem(nullptr, nullptr, 0));
  if (!signedReq)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to create request SECItem"));

  /* Sign */
  {
    /* Create arena for signing */
    c_unique_ptr<PLArenaPool> arena
      = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to create arena"));

    /* DER encode the request */
    auto encoding = SEC_ASN1EncodeItem(arena.get(), nullptr,
      cr, SEC_ASN1_GET(CERT_CertificateRequestTemplate));
    if (!encoding)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "DER encoding of request failed"));

    /* Get the signing algorithm */
    auto signAlgTag = SEC_GetSignatureAlgorithmOidTag(keyType, hashAlgTag);
    if (signAlgTag == SEC_OID_UNKNOWN)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unknown key or hash type"));

    /* Sign the request */
    if (SEC_DerSignData(arena.get(), signedReq.get(), encoding->data,
      encoding->len, privKey, signAlgTag))
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Signing of data failed"));
  }

  return signedReq;
}

PRTime offsetTime(const PRTime& time, ValidityTimeUnit unit, int amt)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(time, PR_GMTParameters, &explodedTime);
  switch (unit)
  {
  case mist::crypto::ValidityTimeUnit::Sec:
    explodedTime.tm_sec += amt;
    break;
  case mist::crypto::ValidityTimeUnit::Min:
    explodedTime.tm_min += amt;
    break;
  case mist::crypto::ValidityTimeUnit::Hour:
    explodedTime.tm_hour += amt;
    break;
  case mist::crypto::ValidityTimeUnit::Day:
    explodedTime.tm_mday += amt;
    break;
  case mist::crypto::ValidityTimeUnit::Month:
    explodedTime.tm_month += amt;
    break;
  case mist::crypto::ValidityTimeUnit::Year:
    explodedTime.tm_year += amt;
    break;
  }
  return PR_ImplodeTime(&explodedTime);
}

c_unique_ptr<CERTValidity>
makeValidity(ValidityTimeUnit validityUnit, int validity,
  ValidityTimeUnit warpUnit = ValidityTimeUnit::Month, int warp = 0)
{
  PRTime now = PR_Now();
  if (warp)
    now = offsetTime(now, warpUnit, warp);
  PRTime after = offsetTime(now, validityUnit, validity);
  return to_unique(CERT_CreateValidity(now, after));
}

/* Adapted from certutil.c */
unsigned int
makeSerialNumberFromNow()
{
  unsigned int serialNumber;
  {
    PRTime now = PR_Now();
    LL_USHR(now, now, 19);
    LL_L2UI(serialNumber, now);
  }
  return serialNumber;
}

c_unique_ptr<CERTCertificate>
makeCert(CERTCertificateRequest* req, CERTName* issuer,
  CERTValidity* validity, unsigned int serialNumber)
{
  auto cert = to_unique(
    CERT_CreateCertificate(serialNumber, issuer, validity, req));
  if (!cert)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to create certificate"));
  return cert;
}

void
setCertVersion(CERTCertificate* cert, int certVersion)
{
  switch (certVersion)
  {
  case SEC_CERTIFICATE_VERSION_1:
    /* The initial version for x509 certificates is version one
    * and this default value must be an implicit DER encoding. */
    cert->version.data = NULL;
    cert->version.len = 0;
    break;
  case SEC_CERTIFICATE_VERSION_2:
  case SEC_CERTIFICATE_VERSION_3:
  case 3: /* unspecified format (would be version 4 certificate). */
    *(cert->version.data) = certVersion;
    cert->version.len = 1;
    break;
  default:
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unknown certificate version"));
  }
}

void
signCert(CERTCertificate* cert, SECOidTag hashAlgTag,
  SECKEYPrivateKey* privKey)
{
  PLArenaPool* arena = cert->arena;

  /* Set signature algorithm */
  SECOidTag algID;
  {
    algID = SEC_GetSignatureAlgorithmOidTag(privKey->keyType, hashAlgTag);
    if (algID == SEC_OID_UNKNOWN)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unknown key or hash type"));

    if (SECOID_SetAlgorithmID(arena, &cert->signature, algID, 0)
      != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
          "Unable to set signature algorithm ID"));
  }

  /* DER encode certificate */
  SECItem der;
  {
    der.len = 0;
    der.data = nullptr;
    if (!SEC_ASN1EncodeItem(arena, &der, cert,
      SEC_ASN1_GET(CERT_CertificateTemplate)))
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to encode certificate"));
  }

  if (SEC_DerSignData(arena, &cert->derCert, der.data, der.len, privKey,
    algID) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to sign encoded certificate data"));
}

c_unique_ptr<CERTCertificate>
createSelfSignedCert(CERTName* subject, SECKEYPrivateKey* privKey,
  SECKEYPublicKey* pubKey, SECOidTag hashAlgTag, unsigned int serialNumber,
  CERTValidity *validity, int certVersion)
{
  auto certReq = makeCertReq(pubKey, subject);
  auto subjectCert = makeCert(certReq.get(), subject,
    validity, serialNumber);
  setCertVersion(subjectCert.get(), certVersion);
  signCert(subjectCert.get(), hashAlgTag, privKey);

  return subjectCert;
}
} // namespace

std::string
createP12TestRsaPrivateKeyAndCert(io::SSLContext& sslCtx,
  const std::string& password, const std::string& subject, int size,
  ValidityTimeUnit validityUnit, int validityNumber)
{
  /* Temporary nickname for the duration of the key and cert creation */
  std::string nickname = nss::createTemporaryNickname();

  /* Create private key */
  auto keyPair = nss::createRsaKeyPair(nickname, size);

  /* Create the self-signed certificate */
  c_unique_ptr<CERTCertificate> cert;
  {
    auto subjectItem = to_unique(CERT_AsciiToName(subject.c_str()));
    SECOidTag hashAlgTag = SEC_OID_UNKNOWN;
    unsigned int serialNumber = crypto::makeSerialNumberFromNow();
    auto validity = crypto::makeValidity(validityUnit, validityNumber);
    int certVersion = 3;

    cert = createSelfSignedCert(subjectItem.get(), keyPair.first.get(),
      keyPair.second.get(), hashAlgTag, serialNumber, validity.get(),
      certVersion);
  }

  /* Import cert with same nickname as private and public key
  to associate them*/
  nss::importCert(nickname, cert.get());

  SECOidTag cipher = SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC;
  SECOidTag certCipher = SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC;

  auto data = nss::exportPKCS12(nickname, cipher, certCipher, password);

  /* Remove certificate and key */
  PK11_DeleteTokenCertAndKey(cert.get(), nullptr);

  return data;
}

std::string publicKeyDerToPem(const std::vector<std::uint8_t>& derPublicKey)
{
  return nss::pemEncodePubKey(nss::decodeDerPublicKey(derPublicKey).get());
}

std::vector<std::uint8_t> publicKeyPemToDer(const std::string& pemPublicKey)
{
  return nss::derEncodePubKey(nss::decodePemPublicKey(pemPublicKey).get());
}

std::vector<std::uint8_t>
base64Decode(const std::string& src)
{
  return nss::base64Decode(src);
}

std::string
base64Encode(const std::vector<std::uint8_t>& src)
{
  return nss::base64Encode(src);
}

std::vector<std::uint8_t>
stringToBuffer(const std::string& src)
{
  auto data(reinterpret_cast<const std::uint8_t*>(src.data()));
  return std::vector<std::uint8_t>(data, data + src.length());
}

std::string
bufferToString(const std::vector<std::uint8_t>& src)
{
  auto data(reinterpret_cast<const char*>(src.data()));
  return std::string(data, data + src.size());
}




} // namespace crypto
} // namespace mist
