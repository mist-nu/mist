#ifndef __MIST_INCLUDE_NSS_PUBKEY_HPP__
#define __MIST_INCLUDE_NSS_PUBKEY_HPP__

#include <memory>
#include <string>

#include <cert.h>
#include <pk11priv.h>
#include <pk11pub.h>

#include "memory/nss.hpp"

namespace mist
{
namespace nss
{

std::vector<std::uint8_t> base64Decode(const std::string& src);

std::string base64Encode(const std::vector<std::uint8_t>& src);

std::vector<std::uint8_t> derEncodePubKey(SECKEYPublicKey* key);

std::string pemEncodePubKey(SECKEYPublicKey* key);

std::vector<std::uint8_t> derEncodeCertPubKey(CERTCertificate* cert);

std::string pemEncodeCertPubKey(CERTCertificate* cert);

c_unique_ptr<SECKEYPublicKey> certPubKey(CERTCertificate* cert);

c_unique_ptr<SECKEYPublicKey>
decodeDerPublicKey(const std::vector<std::uint8_t>& derPublicKey);

c_unique_ptr<SECKEYPublicKey>
decodePemPublicKey(const std::string& pemPublicKey);

} // namespace nss
} // namespace mist

#endif
