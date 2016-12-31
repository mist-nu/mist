/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#include <memory>
#include <string>
#include <vector>

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <base64.h>
#include <cert.h>
#include <pk11priv.h>
#include <pk11pub.h>
#include <secerr.h>
#include <sechash.h>
#include <secitem.h>

#include "crypto/hash.hpp"

#include "error/mist.hpp"
#include "error/nss.hpp"

#include "memory/nss.hpp"

#include "nss/nss_base.hpp"
#include "nss/pubkey.hpp"

namespace mist
{
namespace nss
{

/* Taken directly from NSS source */
SECStatus
SECU_FileToItem(SECItem *dst, PRFileDesc *src)
{
  PRFileInfo info;
  PRInt32 numBytes;
  PRStatus prStatus;

  prStatus = PR_GetOpenFileInfo(src, &info);

  if (prStatus != PR_SUCCESS) {
    PORT_SetError(SEC_ERROR_IO);
    return SECFailure;
  }

  /* XXX workaround for 3.1, not all utils zero dst before sending */
  dst->data = 0;
  if (!SECITEM_AllocItem(NULL, dst, info.size))
    goto loser;

  numBytes = PR_Read(src, dst->data, info.size);
  if (numBytes != info.size) {
    PORT_SetError(SEC_ERROR_IO);
    goto loser;
  }

  return SECSuccess;
loser:
  SECITEM_FreeItem(dst, PR_FALSE);
  return SECFailure;
}

/* Taken directly from NSS source */
SECStatus
SECU_ReadDERFromFile(SECItem *der, PRFileDesc *inFile, PRBool ascii)
{
  SECStatus rv;
  if (ascii) {
    /* First convert ascii to binary */
    SECItem filedata;
    char *asc, *body;

    /* Read in ascii data */
    rv = SECU_FileToItem(&filedata, inFile);
    asc = (char *)filedata.data;
    if (!asc) {
      fprintf(stderr, "unable to read data from input file\n");
      return SECFailure;
    }

    /* check for headers and trailers and remove them */
    if ((body = strstr(asc, "-----BEGIN")) != NULL) {
      char *trailer = NULL;
      asc = body;
      body = PORT_Strchr(body, '\n');
      if (!body)
        body = PORT_Strchr(asc, '\r'); /* maybe this is a MAC file */
      if (body)
        trailer = strstr(++body, "-----END");
      if (trailer != NULL) {
        *trailer = '\0';
      } else {
        fprintf(stderr, "input has header but no trailer\n");
        PORT_Free(filedata.data);
        return SECFailure;
      }
    } else {
      body = asc;
    }

    /* Convert to binary */
    rv = ATOB_ConvertAsciiToItem(der, body);
    if (rv) {
      return SECFailure;
    }

    PORT_Free(filedata.data);
  } else {
    /* Read in binary der */
    rv = SECU_FileToItem(der, inFile);
    if (rv) {
      return SECFailure;
    }
  }
  return SECSuccess;
}

// http://stackoverflow.com/a/8584708
const char *findstring(const char *begin, const char *end,
  const char *needle)
{
  size_t needle_length = strlen(needle);
  size_t i;

  for (i = 0; ; i++)
  {
    if (i + needle_length > end - begin)
    {
      return NULL;
    }

    if (strncmp(&begin[i], needle, needle_length) == 0)
    {
      return &begin[i];
    }
  }
  return NULL;
}

/* Modified from NSS source */
std::vector<std::uint8_t>
fromPem(std::string pemData)
{
  const char* begin = reinterpret_cast<const char*>(pemData.data());
  const char* end = reinterpret_cast<const char*>(pemData.data() + pemData.length());
  const char* asc = begin;
  const char* body = nullptr;

  /* check for headers and trailers and remove them */
  if ((body = findstring(asc, end, "-----BEGIN")) != nullptr) {
    const char *trailer = nullptr;
    asc = body;
    body = findstring(body, end, "\n");
    if (!body)
        body = findstring(asc, end, "\r");
    if (!body)
        BOOST_THROW_EXCEPTION(boost::system::system_error(make_mist_error(MIST_ERR_ASSERTION),
            "Unable to find a newline"));
    if (body)
      end = findstring(++body, end, "-----END");
    if (end == nullptr) {
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_mist_error(MIST_ERR_ASSERTION),
        "Unable to find ASCII trailer"));
    }
    begin = body;
  }

  std::string text(begin, end - begin);
  char* ascii = const_cast<char*>(text.c_str());

  /* Convert to binary */
  c_unique_ptr<SECItem> der = to_unique(SECITEM_AllocItem(nullptr, nullptr, 0));

  if (ATOB_ConvertAsciiToItem(der.get(), ascii) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to convert ASCII"));

  return std::vector<std::uint8_t>(der->data, der->data + der->len);
}

std::vector<std::uint8_t>
base64Decode(const std::string& src)
{
  c_unique_ptr<SECItem> bin = to_unique(SECITEM_AllocItem(nullptr, nullptr, 0));
  if (ATOB_ConvertAsciiToItem(bin.get(), src.c_str()) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to convert ASCII"));
  return std::vector<std::uint8_t>(bin->data, bin->data + bin->len);
}

std::string
base64Encode(const std::vector<std::uint8_t>& src)
{
  SECItem bin{ siAsciiString,
      reinterpret_cast<unsigned char*>(const_cast<std::uint8_t*>(src.data())),
      static_cast<unsigned int>(src.size()) };
  return std::string(BTOA_ConvertItemToAscii(&bin));
}
  
std::vector<std::uint8_t>
derEncodePubKey(SECKEYPublicKey* key)
{
  auto derPubKey = to_unique(SECKEY_EncodeDERSubjectPublicKeyInfo(key));
  return std::vector<std::uint8_t>(
    derPubKey->data,
    derPubKey->data + derPubKey->len);
}

std::string
pemEncodePubKey(SECKEYPublicKey* key)
{
  auto derPubKey = to_unique(SECKEY_EncodeDERSubjectPublicKeyInfo(key));
  return "-----BEGIN PUBLIC KEY-----\n"
    + std::string(BTOA_ConvertItemToAscii(derPubKey.get()))
    + "\n-----END PUBLIC KEY-----\n";
}

std::vector<std::uint8_t>
derEncodeCertPubKey(CERTCertificate* cert)
{
  auto pubKey = to_unique(CERT_ExtractPublicKey(cert));
  return derEncodePubKey(pubKey.get());
}

std::string
pemEncodeCertPubKey(CERTCertificate* cert)
{
  auto pubKey = to_unique(CERT_ExtractPublicKey(cert));
  return pemEncodePubKey(pubKey.get());
}

std::vector<std::uint8_t>
pubKeyHash(SECKEYPublicKey* key)
{
  auto derPubKey = to_unique(SECKEY_EncodeDERSubjectPublicKeyInfo(key));
  return crypto::hash_sha3_256()->finalize(derPubKey->data,
    derPubKey->data + derPubKey->len);
}

c_unique_ptr<SECKEYPublicKey>
certPubKey(CERTCertificate* cert)
{
  return to_unique(CERT_ExtractPublicKey(cert));
}

std::vector<std::uint8_t>
certPubKeyHash(CERTCertificate* cert)
{
  auto pubKey = to_unique(CERT_ExtractPublicKey(cert));
  return pubKeyHash(pubKey.get());
}

c_unique_ptr<SECKEYPublicKey>
decodeDerPublicKey(const std::vector<std::uint8_t>& derPublicKey)
{
  auto publicKeyInfo = to_unique(SECKEY_DecodeDERSubjectPublicKeyInfo(
    nss::toSecItem(derPublicKey).get()));
  if (!publicKeyInfo)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to decode public key"));

  auto publicKey = to_unique(SECKEY_ExtractPublicKey(publicKeyInfo.get()));
  if (!publicKey)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to extract public key"));

  return publicKey;
}

c_unique_ptr<SECKEYPublicKey>
decodePemPublicKey(const std::string& pemPublicKey)
{
  return decodeDerPublicKey(fromPem(pemPublicKey));
}

} // namespace nss
} // namespace mist
